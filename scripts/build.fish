#!/usr/bin/env fish

set BASE_DIR (dirname (status dirname))

set BUILD_DIR $BASE_DIR/build/Debug

# configure
cmake -S $BASE_DIR -B $BUILD_DIR

# build
cmake --build $BUILD_DIR --target all
