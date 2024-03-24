#!/usr/bin/env fish

set BASE_DIR (dirname (status dirname))

set BUILD_DIRS $BASE_DIR/build

set EXE (find $BASE_DIR -path "$BUILD_DIRS*" -name 'fge' -type f)

if test (count $EXE) -eq 0
	echo "executable not found in any of the these dirs: $BUILD_DIRS. Did you BUILD the project?" >&2
	exit 1
end

$EXE[1]
