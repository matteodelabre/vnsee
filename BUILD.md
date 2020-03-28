# Building

## Preparation

Download the [reMarkable toolchain](https://remarkable.engineering/), install it and source it.

```sh
curl https://remarkable.engineering/oecore-x86_64-cortexa9hf-neon-toolchain-zero-gravitas-1.8-23.9.2019.sh -o install-toolchain.sh
chmod u+x install-toolchain.sh
./install-toolchain.sh
```

Clone this repository and change directory into it.

```sh
git clone git@forge.delab.re:matteo/rmvncclient.git
cd rmvncclient
```

Create a directory for build artifacts.

```sh
mkdir build
cd build
```

## Build `libvncserver`

In this section, we will build `libvncserver`, which is a required dependency of `rmvncclient`, for the reMarkable.
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

Create a directory in which build artifacts for `libvncserver` will be placed.

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

```txt
-- Configuring done
-- Generating done
-- Build files have been written to: (...)/rmvncclient/build/libvncserver/build/Release
```

Run build.

```sh
make
```

Install the libraries into your system. They will be installed into the reMarkable SDK, which is safely outside of your OS package manager’s files.

```sh
sudo make install
```

## Build `rmvncclient`

Go back to the main build directory and create a new build directory for `rmvncclient` itself.

```sh
cd ../../..
mkdir Release
cd Release
```

Run CMake configuration.

```sh
cmake ../.. -DCMAKE_BUILD_TYPE=Release
```

The output should not contain any error and should end with those lines:

```txt
-- Configuring done
-- Generating done
-- Build files have been written to: /home/matteo/Projects/rmvncclient/build/Release
```

Run build.

```sh
make
```

You now have a `rmvncclient` executable ready to be executed on your reMarkable!
