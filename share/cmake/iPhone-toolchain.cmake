
SET(CMAKE_CROSSCOMPILING 1)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_SYSTEM_PROCESSOR arm)

# TODO, this gets set to FALSE in the 'Generic' platform but setting
# it again here doesn't seem to do anything. For now we just build a
# static lib
# SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

SET(IPHONE_SDK "4.3")
SET(IPHONE_ROOT "/Developer/Platforms/iPhoneOS.platform/Developer")
SET(IPHONE_SDK_ROOT "${IPHONE_ROOT}/SDKs/iPhoneOS${IPHONE_SDK}.sdk")
SET(CMAKE_OSX_SYSROOT "${IPHONE_SDK_ROOT}")
SET(CMAKE_OSX_ARCHITECTURES armv7)

SET(CMAKE_C_COMPILER "${IPHONE_ROOT}/usr/bin/gcc-4.2")
SET(CMAKE_CXX_COMPILER "${IPHONE_ROOT}/usr/bin/g++-4.2")

INCLUDE_DIRECTORIES("${IPHONE_SDK_ROOT}/usr/include")
INCLUDE_DIRECTORIES("${IPHONE_SDK_ROOT}/usr/include/c++/4.2.1")
INCLUDE_DIRECTORIES("${IPHONE_SDK_ROOT}/usr/include/c++/4.2.1/armv7-apple-darwin10")

LINK_DIRECTORIES("${IPHONE_SDK_ROOT}/usr/lib")
LINK_DIRECTORIES("${IPHONE_SDK_ROOT}/usr/lib/system")

# TODO: do we need this flags
# -pipe, -no-cpp-precomp, -miphoneos-version-min=${IPHONE_SDK}

SET(CMAKE_FIND_ROOT_PATH "${IPHONE_SDK_ROOT}" "${IPHONE_ROOT}")
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(IPHONE_BUILD TRUE)
