CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(SharemindLibSortNetwork VERSION 0.3.0 LANGUAGES CXX)

INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/config.local" OPTIONAL)
INCLUDE("${CMAKE_CURRENT_BINARY_DIR}/config.local" OPTIONAL)

FIND_PACKAGE(SharemindCMakeHelpers 1.6 REQUIRED)

FIND_PACKAGE(SharemindCxxHeaders 0.8.0 REQUIRED)

# Headers:
FILE(GLOB SharemindLibSortNetwork_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
ADD_CUSTOM_TARGET("include_LIBSORTNETWORK_HEADERS_in_IDE"
    SOURCES ${SharemindLibSortNetwork_HEADERS})
INSTALL(FILES ${SharemindLibSortNetwork_HEADERS}
        DESTINATION "include/sharemind/libsortnetwork"
        COMPONENT "dev")


# The library:
FILE(GLOB_RECURSE SharemindLibSortNetwork_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
SharemindAddSharedLibrary(LibSortNetwork
    OUTPUT_NAME "sharemind_sortnetwork"
    SOURCES ${SharemindLibSortNetwork_SOURCES}
)
TARGET_LINK_LIBRARIES(LibSortNetwork PUBLIC "Sharemind::CxxHeaders")
TARGET_INCLUDE_DIRECTORIES(LibSortNetwork
    INTERFACE
        # $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> # TODO
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
    )
SharemindCreateCMakeFindFilesForTarget(LibSortNetwork
    DEPENDENCIES
        "SharemindCxxHeaders 0.8.0"
    )


# Tests:
FILE(GLOB LibSortNetwork_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/Test*.cpp")
FOREACH(testFile IN LISTS LibSortNetwork_TESTS)
    GET_FILENAME_COMPONENT(testName "${testFile}" NAME_WE)
    SharemindAddTest("${testName}" SOURCES "${testFile}")
    TARGET_LINK_LIBRARIES("${testName}" PRIVATE LibSortNetwork)
ENDFOREACH()


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
        "libsharemind-cxxheaders-dev (>= 0.8.0)"
)
SharemindAddComponentPackage("debug"
    NAME "libsharemind-sortnetwork-dbg"
    DESCRIPTION "Sharemind SortNetwork library debug symbols"
    DEB_SECTION "debug"
    DEB_DEPENDS
        "libsharemind-sortnetwork (= ${SharemindLibSortNetwork_DEB_lib_PACKAGE_VERSION})"
)
SharemindPackagingFinalize()
