add_library(model
	model.cpp
	model_impl.cpp
	func_network.cpp
	func_network_impl.cpp
	function.cpp
)

target_link_libraries(model PUBLIC cpp_flags)

target_include_directories(model PUBLIC include)

find_package(exprtk REQUIRED)

target_link_libraries(model PUBLIC
	shared
)
