CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(SharemindLibSortNetwork VERSION 0.2.0 LANGUAGES C)

INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/config.local" OPTIONAL)
INCLUDE("${CMAKE_CURRENT_BINARY_DIR}/config.local" OPTIONAL)

FIND_PACKAGE(SharemindCMakeHelpers 1.3 REQUIRED)


# Headers:
FILE(GLOB_RECURSE SharemindLibSortNetwork_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
ADD_CUSTOM_TARGET("include_LIBSORTNETWORK_HEADERS_in_IDE"
    SOURCES ${SharemindLibSortNetwork_HEADERS})
SET(SharemindLibSortNetwork_INSTALL_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/comparator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stage.h"
)
INSTALL(FILES ${SharemindLibSortNetwork_INSTALL_HEADERS}
        DESTINATION "include/sharemind/libsortnetwork"
        COMPONENT "dev")


# The library:
SharemindSetC99CompileOptions()
FILE(GLOB_RECURSE SharemindLibSortNetwork_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
SharemindAddSharedLibrary(libsortnetwork
    OUTPUT_NAME "sharemind_sortnetwork"
    SOURCES
        ${SharemindLibSortNetwork_SOURCES}
    COMPILE_DEFINITIONS
        "_ISOC99_SOURCE=1"
        "_POSIX_C_SOURCE=200809L"
        "_XOPEN_SOURCE=700"
)


# Install CMake files:
SharemindCreateCMakeFindFiles(
    INCLUDE_DIRS
        "${CMAKE_INSTALL_PREFIX}/include"
    LIBRARIES
        "-L${CMAKE_INSTALL_PREFIX}/lib"
        "-lsharemind_sortnetwork"
)


# Packaging:
SharemindSetupPackaging()
SharemindAddComponentPackage("lib"
    NAME "libsharemind-sortnetwork"
    DESCRIPTION "Sharemind SortNetwork library"
    DEB_SECTION "libs"
    DEB_DEPENDS
        "libc6 (>= 2.19)"
)
SharemindAddComponentPackage("dev"
    NAME "libsharemind-sortnetwork-dev"
    DESCRIPTION "Sharemind SortNetwork library development headers"
    DEB_SECTION "libdevel"
    DEB_DEPENDS
        "libsharemind-sortnetwork (= ${SharemindLibSortNetwork_DEB_lib_PACKAGE_VERSION})"
        "libc6-dev (>= 2.19)"
)
SharemindAddComponentPackage("debug"
    NAME "libsharemind-sortnetwork-dbg"
    DESCRIPTION "Sharemind SortNetwork library debug symbols"
    DEB_SECTION "debug"
    DEB_DEPENDS
        "libsharemind-sortnetwork (= ${SharemindLibSortNetwork_DEB_lib_PACKAGE_VERSION})"
)
SharemindPackagingFinalize()
