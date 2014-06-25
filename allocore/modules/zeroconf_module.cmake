set(BUILD_ZEROCONF 1)


if(BUILD_ZEROCONF)
message("Building Zerconf module.")


if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND ALLOCORE_SRC
    src/protocol/al_Zeroconf_OSX.mm)
    message("Building Zeroconf module.")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  find_package(Threads QUIET)
  find_library(AVAHI_COMMON_LIBRARY NAMES avahi-common)
  find_library(AVAHI_CLIENT_LIBRARY NAMES avahi-client)
  if(CMAKE_THREAD_LIBS_INIT AND AVAHI_COMMON_LIBRARY AND AVAHI_CLIENT_LIBRARY)
  list(APPEND ALLOCORE_LINK_LIBRARIES ${AVAHI_COMMON_LIBRARY} ${AVAHI_CLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
  list(APPEND ALLOCORE_SRC
    src/protocol/al_Zeroconf.cpp)
  message("Building Zeroconf module.")
  else()
    message("NOT Building Zeroconf module. Pthreads, avahi-common and avahi-client required.")
  endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message("NOT Building Zeroconf module. (Not supported on Windows)")
endif()

#list(APPEND ALLOCORE_DEP_INCLUDE_DIRS
#  ${GLUT_INCLUDE_DIR})

#list(APPEND ALLOCORE_LINK_LIBRARIES
#  ${GLUT_LIBRARY})

else()
message("NOT Building Zeroconf module.")
endif(BUILD_ZEROCONF)