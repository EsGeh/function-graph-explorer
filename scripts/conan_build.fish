#!/usr/bin/env fish

set BASE_DIR (dirname (status dirname))

set BUILD_TYPE Release
set BUILD_DIR $BASE_DIR/build/conan
set CONFIGURE_ARGS
set BUILD_ARGS

########################
# Parse Arguments
########################

function print_help
	echo "usage: "(status basename)" OPTIONS [-- CONFIGURE_FLAGS [-- BUILD_ARGS]]"
	echo
	echo "OPTIONS"
	echo "--build BUILD_TYPE. default: $BUILD_TYPE"
end

function validate_build_type
	if test "$_flag_value" = "Debug" -o "$_flag_value" = "Release"
		return 0
	end
	echo "Unknown build type: '$_flag_value'"
	return 1
end

argparse \
	'h/help' \
	'b/build=!validate_build_type' \
	-- $argv
and begin
	if set --query _flag_build
		set BUILD_TYPE $_flag_build
	end
end
or exit 1

set block 0
for arg in $argv
	echo "arg: $arg"
	if test $arg = "--"
		set block 1
		continue
	end
	if test $block -eq 0
		set --append CONFIGURE_ARGS $arg
	else 
		set --append BUILD_ARGS $arg
	end
end

set BUILD_DIR $BUILD_DIR/$BUILD_TYPE

########################
# Actual Script
########################

echo "BUILD_TYPE: '$BUILD_TYPE'"
echo "CONFIGURE_ARGS: '$CONFIGURE_ARGS'"
echo "BUILD_ARGS: '$BUILD_ARGS'"
echo "BUILD_DIR: '$BUILD_DIR'"

# install
conan install $BASE_DIR --output-folder "$BUILD_DIR" --build missing --settings build_type=$BUILD_TYPE

# configure
# temporary preprocessor definitions
# can be added like this (for now):
# 	... -DCMAKE_CXX_FLAGS='-DDEBUG_CONCURRENCY'
# NOTE: there may be better ways.
and begin
	set cmd cmake -B $BUILD_DIR -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CONFIGURE_ARGS
	echo "execute: '$cmd'"
	$cmd
end


# create './compile_commands.json'
# which helps editors and IDEs
# finding source files:
and begin
	set cmd cmake --build $BUILD_DIR --target CopyCompileCommands
	echo "execute: '$cmd'"
	$cmd
end

# build
and begin
	set cmd cmake --build $BUILD_DIR $BUILD_ARGS
	echo "execute: '$cmd'"
	$cmd
end
