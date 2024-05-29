add_library(shared
	utils.cpp
	data.cpp
	include/fge/shared/concurrency_utils.h
)

target_link_libraries(shared PUBLIC cpp_flags)

target_include_directories(shared PUBLIC include)
target_include_directories(shared PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)

find_package(exprtk REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core)

configure_file(
	config.h.in
	include/fge/shared/config.h
	@ONLY
)

target_link_libraries(shared PUBLIC exprtk::exprtk)
# because of QString... :-(
target_link_libraries(shared PUBLIC Qt6::Core)
