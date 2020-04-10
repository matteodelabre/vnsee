# Building

Download the [reMarkable toolchain](https://remarkable.engineering/), install it and source it.

```sh
curl https://remarkable.engineering/oecore-x86_64-cortexa9hf-neon-toolchain-zero-gravitas-1.8-23.9.2019.sh -o install-toolchain.sh
chmod u+x install-toolchain.sh
./install-toolchain.sh
source /usr/local/oecore-x86_64/environment-setup-cortexa9hf-neon-oe-linux-gnueabi
```

Download the `OEToolchainConfig.cmake` file that is missing from the toolchain and install it in `/sysroots/x86_64-oesdk-linux/usr/share/cmake` directory of the toolchain.

```sh
curl https://github.com/openembedded/openembedded-core/blob/master/meta/recipes-devtools/cmake/cmake/OEToolchainConfig.cmake -o OEToolchainConfig.cmake
sudo mkdir -p /usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake
sudo mv OEToolchainConfig.cmake /usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake
```

Clone this repository with its dependencies and change directory into it.

```sh
git clone --recursive https://github.com/matteodelabre/rmvncclient
cd rmvncclient
```

Create a directory for build artifacts.

```sh
mkdir -p build/Release
cd build/Release
```

Run CMake configuration.

```sh
cmake ../.. -DCMAKE_BUILD_TYPE=Release
```

The output should not contain any error and should end with those lines:

```txt
-- Configuring done
-- Generating done
-- Build files have been written to: …/rmvncclient/build/Release
```

Run build.

```sh
make
```

You now have a `rmvncclient` executable ready to be executed on your reMarkable!
