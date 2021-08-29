# Build guide

_This build guide uses the [Toltec Docker toolchain](https://github.com/toltec-dev/toolchain), which requires a working Docker install._

Start by cloning this repository with its dependencies, then change directory into it.

```sh
git clone --recursive https://github.com/matteodelabre/vnsee
cd vnsee
```

Use the following command to spin up a container and run a build in the `build/Release` subdirectory.

```sh
docker container run -it --rm \
    --mount type=bind,src="$(realpath .)",dst=/src \
    ghcr.io/toltec-dev/base:v2.1 \
    bash -c "cmake \
        -DCMAKE_TOOLCHAIN_FILE=/usr/share/cmake/arm-linux-gnueabihf.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -S /src -B /src/build/Release \
    && cmake --build /src/build/Release"
```

This command will first run the CMake configuration command, which will automatically locate the appropriate compiler and determine flags.
This step should end with the following lines:

```
...
-- Configuring done
-- Generating done
-- Build files have been written to: /src/build/Release
```

Right after the configuration step has finished, the command will start the actual build.
You should see CMake’s colorful output logging the files it compiles, with lines similar to this one:

```
...
[ 16%] Building CXX object CMakeFiles/vnsee.dir/src/main.cpp.o
...
```

When this step completes, you should have a working `vnsee` executable in the `build/Release` subdirectory, ready to be executed on your reMarkable!
