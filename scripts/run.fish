#!/usr/bin/env fish

set BASE_DIR (dirname (status dirname))
set BUILD_DIRS $BASE_DIR/build/conan
set BUILD_TYPE Release

########################
# Parse Arguments
########################

function print_help
	echo "usage: "(status basename)" OPTIONS"
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

########################
# Actual Script
########################

echo "BUILD_TYPE: '$BUILD_TYPE'"

set EXE (find $BASE_DIR -path "$BUILD_DIRS/$BUILD_TYPE*" -name 'fge' -type f)

if test (count $EXE) -eq 0
	echo "executable not found in any of the these dirs: '$BUILD_DIRS/$BUILD_TYPE'. Did you BUILD the project?" >&2
	exit 1
end

echo "running '$EXE[1]'"
$EXE[1] &

sleep 1

jack_connect fge:out system:playback_1
jack_connect fge:out system:playback_2

wait $last_pid
