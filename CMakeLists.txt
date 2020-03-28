cmake_minimum_required(VERSION 3.4)
project(revnc C)

# Find libvncclient
find_package(PkgConfig REQUIRED)
pkg_check_modules(VNCCLIENT REQUIRED IMPORTED_TARGET libvncclient)
set_property(TARGET PkgConfig::VNCCLIENT PROPERTY
    INTERFACE_LINK_LIBRARIES "${VNCCLIENT_STATIC_LIBRARIES}")

add_executable(revnc
    src/main.c
    src/screen.c
    src/util.c
)

target_link_libraries(revnc PUBLIC PkgConfig::VNCCLIENT)
