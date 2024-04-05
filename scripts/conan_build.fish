#!/usr/bin/env fish

set BASE_DIR (dirname (status dirname))

set BUILD_TYPE Debug
set BUILD_DIR $BASE_DIR/build/conan/$BUILD_TYPE

# configure
conan install $BASE_DIR --output-folder "$BUILD_DIR" --build missing --settings build_type=$BUILD_TYPE

cmake -B $BUILD_DIR -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE

# build
cmake --build $BUILD_DIR $argv

# create './compile_commands.json'
# which helps editors and IDEs
# finding source files:
cmake --build $BUILD_DIR --target CopyCompileCommands
