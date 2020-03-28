# Building

## Preparation

Download the reMarkable SDK, install it and source it.

TODO

Clone the repository and change directory into it.

```sh
git clone git@forge.delab.re:matteo/rmvnc.git
cd rmvnc
```

Create a directory in which build artifacts will be generated.

```sh
mkdir build
cd build
```

## Build libvncserver

In this section, we will build `libvncserver`, which is a required dependency of `rmvnc`, for the reMarkable.
Firstly, clone the repository and change directory into it.

```sh
git clone https://github.com/LibVNC/libvncserver
cd libvncserver
```

Change the first four lines in `libvncclient.pc.cmakein` to the following.

```pc
prefix=/usr
exec_prefix=${prefix}
libdir=${exec_prefix}/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/include
```

Create a directory in which build artifacts (for `libvncserver`) will be generated.

```sh
mkdir -p build/Release
cd build/Release
```

Run CMake configuration.

```sh
cmake ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$SDKTARGETSYSROOT/usr" \
    -DBUILD_SHARED_LIBS=OFF \
    -DWITH_GNUTLS=OFF
```

The output should not contain any error and should end with those lines:

```
-- Configuring done
-- Generating done
-- Build files have been written to: (...)/rmvnc/build/libvncserver/build/Release
```

Run build.

```sh
make
```

Install the libraries into your system. They will be installed into the reMarkable SDK, which is safely outside of your OS package manager’s files.

```sh
sudo make install
```

## Build rmvnc

TODO
