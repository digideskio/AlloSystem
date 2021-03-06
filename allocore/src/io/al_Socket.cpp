/*
TCP:
(PF_INET, PF_INET6), (SOCK_STREAM), (IPPROTO_TCP)
TCP Server:
1) Create a TCP socket
2) Bind socket to the listen port, with a call to bind()
3) Prepare socket to listen for connections with a call to listen()
4) Accepting incoming connections, via a call to accept()
5) Communicate with remote host, which can be done through, e.g., send()
6) Close each socket that was opened, once it is no longer needed, using close()

From http://stackoverflow.com/questions/6189831/whats-the-purpose-of-using-sendto-recvfrom-instead-of-connect-send-recv-with-ud:

It is important to understand that TCP is "Connection Oriented" and UDP is "Connection-less" protocol:

TCP: You need to connect first prior to sending/receiving data to/from a remote host.
UDP: No connection is required. You can send/receive data to/from any host.
Therefore, you will need to use sendto() on UDP socket in order to specify the destination. Similarly, you would want to use recvfrom() to know where the UDP data was received from.

You can actually use connect() on UDP socket as an option. In that case, you can use send()/recv() on the UDP socket to send data to the address specified with the connect() and to receive data only from the address. (The connect() on UDP socket merely sets the default peer address and you can call connect() on UDP socket as many times as you want, and the connect() on UDP socket, of course, does not perform any handshake for connection.)
*/

#include <string>
#include "allocore/io/al_Socket.hpp"
#include "allocore/system/al_Config.h"
#include "allocore/system/al_Printing.hpp"

#define PRINT_SOCKADDR(s)\
	printf("%s %s\n", s->hostname, s->servname);

namespace al{

struct SocketImplBase{
	
	SocketImplBase(uint16_t port=0, const char * address="")
	:	mPort(port), mAddress(address)
	{}

	int mType = 0;
	al_sec mTimeout = -1;
	std::string mAddress;
	uint16_t mPort = 0;
};

} // al::


#if defined AL_WINDOWS

// Note that much of this closely matches POSIX

#include <winsock2.h>
#include <ws2tcpip.h>

// Initialization singleton
struct WsInit{
	WsInit(){
		WSADATA wsaData;
		int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if(0 != iResult){
			printf("WSAStartup failed: %d\n", iResult);
		}
	}
	~WsInit(){ WSACleanup(); }
	static WsInit& get(){
		static WsInit v;
		return v;
	}
};


namespace al {

static const wchar_t * errorString(){
	static wchar_t s[4096];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
					NULL, WSAGetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					s, sizeof(s), NULL);
	return s;
}

class Socket::Impl : public SocketImplBase{
public:

	struct addrinfo * mAddrInfo = NULL;
	SOCKET mSocket = INVALID_SOCKET;

	Impl(){
		WsInit::get();
	}

	Impl(uint16_t port, const char * address, al_sec timeout_, int type)
	:	SocketImplBase(port,address)
	{
		WsInit::get();

		// opens the socket also:
		if(!open(port, address, timeout_, type)){
			AL_WARN("Socket::Impl failed to open port %d / address \"%s\"\n", port, address);
		}
	}
	

	bool open(uint16_t port, std::string address, al_sec timeoutSec, int type){

		close();

		mPort = port;
		mAddress = address;
		mType = type;

		int sockProto = type & 127;
		switch(sockProto){
		case TCP:  sockProto = IPPROTO_TCP; break;
		case UDP:  sockProto = IPPROTO_UDP; break;
		case SCTP: sockProto = IPPROTO_SCTP; break;
		default:;
		}

		int sockType = type & (127<<8);
		switch(sockType){
		case STREAM: sockType = SOCK_STREAM; break;
		case DGRAM:  sockType = SOCK_DGRAM; break;
		case 0: // unspecified; choose sensible default, if possible
			switch(sockProto){
			case TCP:  sockType = SOCK_STREAM; break;
			case UDP:
			case SCTP: sockType = SOCK_DGRAM; break;
			default:;
			}
		}

		int sockFamily = type & (127<<16);
		switch(sockFamily){
		//case 0:		sockFamily = AF_UNSPEC; break; // AF_INET6 if addr=""
		case 0:
		case INET:  sockFamily = AF_INET; break;
		case INET6:
		#ifdef AF_INET6
			sockFamily = AF_INET6; break;
		#else
			AL_WARN("Socket::INET6 not supported on this platform.");
			return false;
		#endif
		default:;
		}

		struct addrinfo hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = sockFamily;
		hints.ai_socktype = sockType;
		hints.ai_protocol = sockProto;
		
		const char * addr;
		if(mAddress.empty()){ // assume this means it's a server
			addr = NULL;
			hints.ai_flags = AI_PASSIVE;
		} else {
			addr = mAddress.c_str();
		}

		// Resolve address and port
		char portAsString[6];
		if(0 !=	getaddrinfo(addr, _itoa(mPort, portAsString, 10), &hints, &mAddrInfo)){
			AL_WARN("failed to resolve %s:%i: %S", address.c_str(), port, errorString());
			close();
			return false;
		}
		//printf("family=%d\n", mAddrInfo->ai_family);

		// Create socket
		mSocket = socket(mAddrInfo->ai_family, mAddrInfo->ai_socktype, mAddrInfo->ai_protocol);
		if(!opened()){
			AL_WARN("failed to create socket at %s:%i: %S", address.c_str(), port, errorString());
			close();
			return false;
		}

		// Set timeout
		timeout(timeoutSec);

		return true;
	}

	void close(){
		if(mAddrInfo){
			freeaddrinfo(mAddrInfo);
			mAddrInfo = NULL;
		}
		if(INVALID_SOCKET != mSocket){
			shutdown(mSocket, SD_BOTH); // SD_SEND, SD_RECEIVE
			closesocket(mSocket);
			mSocket = INVALID_SOCKET;
		}
	}

	bool reopen(){
		return open(mPort, mAddress, mTimeout, mType);
	}

	bool bind(){ // for server-side
		if(opened()){
			if(SOCKET_ERROR == ::bind(mSocket, mAddrInfo->ai_addr, (int)mAddrInfo->ai_addrlen)){
				AL_WARN("unable to bind socket at %s:%i: %S", mAddress.c_str(), mPort, errorString());
				close();
				return false;
			}
		}
		return true;
	}

	bool connect(){ // for client-side
		if(opened()){
			if(SOCKET_ERROR == ::connect(mSocket, mAddrInfo->ai_addr, (int)mAddrInfo->ai_addrlen)){
				AL_WARN("unable to connect socket at %s:%i: %S", mAddress.c_str(), mPort, errorString());
				close();
				return false;
			}
		}
		return true;
	}

	void timeout(al_sec v){
		mTimeout = v;

		DWORD ms = DWORD(v*1000. + 0.5); // Note: DWORD is an unsigned integer
		if(SOCKET_ERROR == ::setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&ms, sizeof(ms))){
			AL_WARN("unable to set snd timeout on socket at %s:%i: %S", mAddress.c_str(), mPort, errorString());
		}
		if(SOCKET_ERROR == ::setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&ms, sizeof(ms))){
			AL_WARN("unable to set rcv timeout on socket at %s:%i: %S", mAddress.c_str(), mPort, errorString());
		}
	}

	bool listen(){
		/* APR_SO_REUSEADDR is useful for a socket listening process.
		It specifies that the "The rules used in validating addresses supplied
		to bind should allow reuse of local addresses." */
		/*apr_socket_opt_set(mSock, APR_SO_REUSEADDR, 1);
		apr_status_t res = apr_socket_listen(mSock, SOMAXCONN);
		if(APR_SUCCESS != res){
			//AL_WARN("Failed to listen on socket");
			return false;
		}*/
		if(SOCKET_ERROR == ::listen(mSocket, SOMAXCONN)){
			AL_WARN("unable to listen at %s:%i: %S", mAddress.c_str(), mPort, errorString());
			return false;
		}
		return true;
	}

	bool accept(Socket::Impl * newSock){
		// TODO
		/*struct sockaddr newSockAddr;
		SOCKET newSOCKET = ::accept(mSocket, &newSockAddr);
		if(INVALID_SOCKET == newSOCKET){
			AL_WARN("unable to accept using listener at %s:%i: %S", mAddress.c_str(), mPort, errorString());
			return false;
		}
		newSock->mSocket = newSOCKET;
		newSock->mAddrInfo.ai_family = newSockAddr.sa_family;
		switch(newSockAddr.sa_family){
		case AF_INET:
			newSock->mAddrInfo.ai_port = ((sockaddr_in *)&newSockAddr)->sin_port;
			newSock->mAddrInfo.ai_port = ((sockaddr_in *)&newSockAddr)->sin_port;
		}
		newSock->close();
		// Inherit timeout from parent
		newSock->timeout(mTimeout);
		return true;*/
		return false;
	}

	bool opened() const {
		return INVALID_SOCKET != mSocket;
	}

	size_t recv(char * buffer, size_t maxlen){
		return ::recv(mSocket, buffer, maxlen, 0);
	}

	size_t send(const char * buffer, size_t len){
		return ::send(mSocket, buffer, len, 0);
	}
};

std::string Socket::hostIP(){
	hostent * host = gethostbyname("");
	if(NULL == host){
		AL_WARN("unable to obtain host IP: ", errorString());
		return "";
	}

	// Note there can be more than one IP address. We loop just to demonstrate how to
	// get all the IPs, even though we return the first.
	int i=0;
	while(host->h_addr_list[i] != 0){
		struct in_addr addr;
		addr.s_addr = *(u_long *)host->h_addr_list[i++];
		return inet_ntoa(addr);
	}
	return "";
}

std::string Socket::hostName(){
	WsInit::get();
	char buf[256] = {0};
	if(SOCKET_ERROR == gethostname(buf, sizeof(buf))){
		AL_WARN("unable to obtain host name: ", errorString());
	}
	return buf;
}

} // al::

#else
#error Socket implementation not defined for this platform.
#endif



// Everything below is common across all platforms
namespace al{

Socket::Socket()
:	mImpl(new Impl)
{}

Socket::Socket(uint16_t port, const char * address, al_sec timeout, int type)
:	mImpl(new Impl(port, address, timeout, type))
{}

Socket::~Socket(){
	close();
	delete mImpl;
}

const std::string& Socket::address() const { return mImpl->mAddress; }

bool Socket::opened() const { return mImpl->opened(); }

uint16_t Socket::port() const { return mImpl->mPort; }

al_sec Socket::timeout() const { return mImpl->mTimeout; }

bool Socket::bind(){ return mImpl->bind(); }

bool Socket::connect(){ return mImpl->connect(); }

void Socket::close(){ mImpl->close(); }

bool Socket::open(uint16_t port, const char * address, al_sec timeout, int type){
	return
		mImpl->open(port, address, timeout, type)
		&& onOpen();
}

void Socket::timeout(al_sec v){
	mImpl->timeout(v);
}

size_t Socket::recv(char * buffer, size_t maxlen){
	return mImpl->recv(buffer, maxlen);
}

size_t Socket::send(const char * buffer, size_t len) {
	return mImpl->send(buffer, len);
}

bool Socket::listen(){
	return mImpl->listen();
}

bool Socket::accept(Socket& sock){
	return mImpl->accept(sock.mImpl);
}

bool SocketClient::onOpen(){
	return connect();
}

bool SocketServer::onOpen(){
	return bind();
}

} // al::
