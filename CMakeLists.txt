cmake_minimum_required(VERSION 3.4)
project(rmvncclient)

# Import libvncserver in static mode
set(BUILD_SHARED_LIBS OFF CACHE STRING "" FORCE)
set(WITH_GNUTLS OFF CACHE STRING "" FORCE)
add_subdirectory(libvncserver)

# Options for building rmvncclient
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

add_executable(rmvncclient
    src/rmioc/input.cpp
    src/rmioc/touch.cpp
    src/rmioc/screen.cpp
    src/client.cpp
    src/client-input.cpp
    src/client-screen.cpp
    src/main.cpp
)

set_property(TARGET rmvncclient PROPERTY CXX_STANDARD 17)
target_link_libraries(rmvncclient PRIVATE vncclient)
target_include_directories(rmvncclient PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/libvncserver
    libvncserver
)
