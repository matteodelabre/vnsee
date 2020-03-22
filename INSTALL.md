
## Build libvncserver

Change the first lines in `libvncclient.pc.cmakein` to:

```pc
prefix=/usr
exec_prefix=${prefix}
libdir=${exec_prefix}/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/include
```

Run:

```
cmake .. \
    -DCMAKE_INSTALL_PREFIX="$SDKTARGETSYSROOT/usr" \
    -DBUILD_SHARED_LIBS=OFF \
    -DWITH_GNUTLS=OFF
make
make install
```


