list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake")
include (FindPkgConfig)

qt_add_library(audio
	jack.cpp
	include/fge/audio/jack.h
)
target_include_directories(audio PUBLIC include)
target_link_libraries(audio PUBLIC cpp_flags)

find_package(Jack REQUIRED)

target_link_libraries(audio PUBLIC shared)
target_link_libraries(audio PUBLIC Jack::Jack)
