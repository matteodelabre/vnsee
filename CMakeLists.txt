cmake_minimum_required(VERSION 3.7)
project(VNSee VERSION 0.3.1)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LibVNCClient libvncclient)

if(NOT LibVNCClient_FOUND)
    message(STATUS "LibVNCClient not installed - using built-in version")
    set(BUILD_SHARED_LIBS OFF CACHE STRING "" FORCE)
    set(WITH_GNUTLS OFF CACHE STRING "" FORCE)
    add_subdirectory(libvncserver)
endif()

# Options for building vnsee
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

add_executable(vnsee
    src/app/buttons.cpp
    src/app/client.cpp
    src/app/pen.cpp
    src/app/screen.cpp
    src/app/touch.cpp
    src/rmioc/buttons.cpp
    src/rmioc/input.cpp
    src/rmioc/pen.cpp
    src/rmioc/screen.cpp
    src/rmioc/touch.cpp
    src/main.cpp
)

if(CMAKE_VERSION VERSION_LESS "3.8")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")
    else()
        message(FATAL_ERROR "Unknown compiler")
    endif()
else()
    set_property(TARGET vnsee PROPERTY CXX_STANDARD 17)
endif()

configure_file(src/config.hpp.in src/config.hpp)
target_include_directories(vnsee PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src)

if(NOT LibVNCClient_FOUND)
    target_link_libraries(vnsee PRIVATE vncclient)
    target_include_directories(vnsee PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/libvncserver libvncserver)
else()
    target_link_libraries(vnsee PUBLIC ${LibVNCClient_LDFLAGS})
    target_include_directories(vnsee PUBLIC ${LibVNCClient_INCLUDE_DIRS})
endif()
