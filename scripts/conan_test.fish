#!/usr/bin/env fish

set BASE_DIR (dirname (status dirname))

set BUILD_TYPE Debug
set BUILD_DIR $BASE_DIR/build/conan/$BUILD_TYPE

# build
$BASE_DIR/scripts/conan_build.fish --target build_tests
and begin
	ctest --test-dir $BUILD_DIR/tests --output-on-failure --exclude-regex 'benchmark' $argv
end
