# Build guide

**This build guide assumes you’re using a Linux system.**

Download the [reMarkable toolchain](https://web.archive.org/web/20201129102245/https://remarkable.engineering/oecore-x86_64-cortexa9hf-neon-toolchain-zero-gravitas-1.8-23.9.2019.sh), install it and source it.

```sh
curl https://web.archive.org/web/20201129102245/https://remarkable.engineering/oecore-x86_64-cortexa9hf-neon-toolchain-zero-gravitas-1.8-23.9.2019.sh -o install-toolchain.sh
chmod u+x install-toolchain.sh
./install-toolchain.sh
source /usr/local/oecore-x86_64/environment-setup-cortexa9hf-neon-oe-linux-gnueabi
```

Download the `OEToolchainConfig.cmake` file that is missing from the toolchain and install it in `/sysroots/x86_64-oesdk-linux/usr/share/cmake` directory of the toolchain.

```sh
curl https://raw.githubusercontent.com/openembedded/openembedded-core/master/meta/recipes-devtools/cmake/cmake/OEToolchainConfig.cmake -o OEToolchainConfig.cmake
sudo mkdir -p /usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake
sudo mv OEToolchainConfig.cmake /usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake
```

Clone this repository with its dependencies and change directory into it.

```sh
git clone --recursive https://github.com/matteodelabre/vnsee
cd vnsee
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
-- Build files have been written to: …/vnsee/build/Release
```

Run build.

```sh
make
```

You now have a `vnsee` executable ready to be executed on your reMarkable!
