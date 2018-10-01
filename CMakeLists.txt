CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(SharemindLibSortNetwork VERSION 0.2.0 LANGUAGES CXX)

INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/config.local" OPTIONAL)
INCLUDE("${CMAKE_CURRENT_BINARY_DIR}/config.local" OPTIONAL)

FIND_PACKAGE(SharemindCMakeHelpers 1.3 REQUIRED)

FIND_PACKAGE(SharemindCxxHeaders 0.5 REQUIRED)

# Headers:
FILE(GLOB SharemindLibSortNetwork_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
ADD_CUSTOM_TARGET("include_LIBSORTNETWORK_HEADERS_in_IDE"
    SOURCES ${SharemindLibSortNetwork_HEADERS})
INSTALL(FILES ${SharemindLibSortNetwork_HEADERS}
        DESTINATION "include/sharemind/libsortnetwork"
        COMPONENT "dev")


# The library:
SharemindSetCxx11CompileOptions()
FILE(GLOB_RECURSE SharemindLibSortNetwork_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
SharemindAddSharedLibrary(libsortnetwork
    OUTPUT_NAME "sharemind_sortnetwork"
    SOURCES
        ${SharemindLibSortNetwork_SOURCES}
    INCLUDE_DIRECTORIES
        "${SharemindCxxHeaders_INCLUDE_DIRS}"
    LEGACY_DEFINITIONS
        "${SharemindCxxHeaders_DEFINITIONS}"
    LINK_LIBRARIES
        "${SharemindCxxHeaders_LIBRARIES}"
)


# Tests:
SharemindAddSimpleTests("${CMAKE_CURRENT_SOURCE_DIR}/tests/Test*.cpp"
    INCLUDE_DIRECTORIES
        "${SharemindCxxHeaders_INCLUDE_DIRS}"
    LEGACY_DEFINITIONS
        "${SharemindCxxHeaders_DEFINITIONS}"
    LINK_LIBRARIES
        "${SharemindCxxHeaders_LIBRARIES}"
        "libsortnetwork"
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
        "libstdc++6 (>= 4.8.0)"
)
SharemindAddComponentPackage("dev"
    NAME "libsharemind-sortnetwork-dev"
    DESCRIPTION "Sharemind SortNetwork library development headers"
    DEB_SECTION "libdevel"
    DEB_DEPENDS
        "libsharemind-sortnetwork (= ${SharemindLibSortNetwork_DEB_lib_PACKAGE_VERSION})"
        "libc6-dev (>= 2.19)"
        "libstdc++-dev"
)
SharemindAddComponentPackage("debug"
    NAME "libsharemind-sortnetwork-dbg"
    DESCRIPTION "Sharemind SortNetwork library debug symbols"
    DEB_SECTION "debug"
    DEB_DEPENDS
        "libsharemind-sortnetwork (= ${SharemindLibSortNetwork_DEB_lib_PACKAGE_VERSION})"
)
SharemindPackagingFinalize()
