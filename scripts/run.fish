#!/usr/bin/env fish

set BASE_DIR (dirname (status dirname))

set BUILD_DIR $BASE_DIR/build/Debug

$BUILD_DIR/qt-test
