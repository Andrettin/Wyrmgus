#!/bin/bash
# Build a binary for Wyrmgus against the Steam runtime

# Error out if we are not at the root of the source directory
if [ ! -d ./src ]
then
    echo -e "The ./src directory seems not to be present."
    exit 1
fi

# Append 32 or 64 to the binary name depending on the arch
archsuffix=$(getconf LONG_BIT)
binname=wyrmsun$archsuffix

cmake \
      -DENABLE_STATIC=OFF \
      -DWITH_X11=ON \
      -DWITH_GEOJSON=OFF \
      $@

# Hack to link lua 5.1 statically
sed -i -e 's/liblua5.1.so/liblua5.1.a/g' CMakeCache.txt

NCPUS_MAX=`/usr/bin/getconf _NPROCESSORS_ONLN`
make -j$NCPUS_MAX

mv -f wyrmgus $binname
