cmake_minimum_required(VERSION 3.4)
project(rmvncclient C)

# Find libvncclient
find_package(PkgConfig REQUIRED)
pkg_check_modules(VNCCLIENT REQUIRED IMPORTED_TARGET libvncclient)
set_property(TARGET PkgConfig::VNCCLIENT PROPERTY
    INTERFACE_LINK_LIBRARIES "${VNCCLIENT_STATIC_LIBRARIES}")

option(TRACE "Print screen updates to stderr" OFF)

if(TRACE)
    add_definitions(-DTRACE)
endif(TRACE)

add_executable(rmvncclient
    src/main.c
    src/screen.c
    src/util.c
)

target_link_libraries(rmvncclient PUBLIC PkgConfig::VNCCLIENT)
