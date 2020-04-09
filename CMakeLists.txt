cmake_minimum_required(VERSION 3.4)
project(rmvncclient)

option(CHECK_INCLUDES "Run include-what-you-use to check #includes" OFF)
option(CHECK_TIDY "Run clang-tidy linter" OFF)
option(TRACE "Print tracing messages on standard output" OFF)

if(CHECK_INCLUDES)
    find_program(IWYU_PATH include-what-you-use)

    if(NOT IWYU_PATH)
        message(FATAL_ERROR "Could not find include-what-you-use")
    endif()

    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
endif()

if(CHECK_TIDY)
    find_program(TIDY_PATH clang-tidy)

    if(NOT TIDY_PATH)
        message(FATAL_ERROR "Could not find clang-tidy")
    endif()

    set(CMAKE_CXX_CLANG_TIDY ${TIDY_PATH};-fix)
endif()

if(TRACE)
    message(STATUS "Trace mode enabled")
    add_definitions(-DTRACE)
endif()

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(VNCCLIENT REQUIRED IMPORTED_TARGET libvncclient)
set_property(TARGET PkgConfig::VNCCLIENT PROPERTY
    INTERFACE_LINK_LIBRARIES "${VNCCLIENT_STATIC_LIBRARIES}")

add_executable(rmvncclient
    src/rmioc/input.cpp
    src/rmioc/screen.cpp
    src/client.cpp
    src/client-input.cpp
    src/client-screen.cpp
    src/main.cpp
)

set_property(TARGET rmvncclient PROPERTY CXX_STANDARD 17)

target_link_libraries(rmvncclient PUBLIC PkgConfig::VNCCLIENT)
