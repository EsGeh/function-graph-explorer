#!/usr/bin/env fish

set BASE_DIR (dirname (status dirname))

set BUILD_DIRS $BASE_DIR/build

for dir in $BUILD_DIRS
	echo "rm -rf '$dir'"
	rm -rf $dir
end
