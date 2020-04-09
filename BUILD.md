# Building

Download the [reMarkable toolchain](https://remarkable.engineering/), install it and source it.

```sh
curl https://remarkable.engineering/oecore-x86_64-cortexa9hf-neon-toolchain-zero-gravitas-1.8-23.9.2019.sh -o install-toolchain.sh
chmod u+x install-toolchain.sh
./install-toolchain.sh
source /usr/local/oecore-x86_64/environment-setup-cortexa9hf-neon-oe-linux-gnueabi
```

Clone this repository with its dependencies and change directory into it.

```sh
git clone --recursive git@forge.delab.re:matteo/rmvncclient.git
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
