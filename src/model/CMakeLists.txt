add_library(model
	model.cpp
	model_impl.cpp
	sampled_func_collection_impl.cpp
	function_collection.cpp
	function_collection_impl.cpp
	function.cpp
)

target_link_libraries(model PUBLIC cpp_flags)

target_include_directories(model PUBLIC include)

find_package(exprtk REQUIRED)

target_link_libraries(model PUBLIC
	shared
)
