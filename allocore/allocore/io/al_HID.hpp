#ifndef INCLUDE_AL_HID_HPP
#define INCLUDE_AL_HID_HPP

#include <wchar.h>
#include <string>
#define AL_HID_BUFFER_SIZE	256

namespace al{

class HID{
public:

	HID();
	~HID();
	
	/// Open a HID device
	
	/// @param[in] vendorID		The Vendor ID (VID) of the device to open.
	/// @param[in] productID	The Product ID (PID) of the device to open.
	/// @param[in] serialNum	The Serial Number of the device to open (optionally NULL).
	/// \returns whether the device was opened.					
	bool open(unsigned short vendorID, unsigned short productID, const wchar_t *serialNumber = NULL);

	/// Close HID device
	void close();


	/// Read an input report from the HID device
	
	/// The first byte will contain the Report number if the device uses 
	/// numbered reports.
	///
	/// @param[in] data		A buffer to put the read data into.
	/// @param[in] length	The number of bytes to read. For devices with
	///						multiple reports, make sure to read an extra byte
	///						for the report number.
	/// \returns the actual number of bytes read and -1 on error.
	int read(unsigned char * data, size_t length);


	/// Get whether the device has been opened
	bool opened() const;

	/// Get manufacturer string
	std::wstring manufacturer() const;
	
	/// Get product string
	std::wstring product() const;
	
	/// Get serial number string
	std::wstring serialNumber() const;

private:
	class Impl;
	Impl * mImpl;
	unsigned char mBuf[AL_HID_BUFFER_SIZE];
};

} // al::
#endif
