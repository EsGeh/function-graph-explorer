enable_testing(true)
include(CTest)

set(TESTS
	testformulafunction
	testmodel
	modelbenchmark
)

add_custom_target(build_tests)
add_dependencies(
	build_tests
	${TESTS}
)

find_package(Qt6 REQUIRED COMPONENTS Test)

######################
# test function:
######################

add_executable(testformulafunction
	EXCLUDE_FROM_ALL
	testfunction.cpp
	testfunction.h
)
set_target_properties(testformulafunction PROPERTIES
	AUTOMOC ON
)
target_link_libraries(testformulafunction PRIVATE Qt6::Test)
target_link_libraries(testformulafunction PRIVATE model)
add_test(testformulafunction testformulafunction)

######################
# test model:
######################

add_executable(testmodel
	EXCLUDE_FROM_ALL
	testmodel.cpp
	testmodel.h
	testutils.h
)
set_target_properties(testmodel PROPERTIES
	AUTOMOC ON
)
target_link_libraries(testmodel PRIVATE Qt6::Test)
target_link_libraries(testmodel PRIVATE model)
add_test(testmodel testmodel)

######################
# Model Benchmark:
######################

add_executable(modelbenchmark
	EXCLUDE_FROM_ALL
	modelbenchmark.cpp
	modelbenchmark.h
	testutils.h
)
set_target_properties(modelbenchmark PROPERTIES
	AUTOMOC ON
)
target_link_libraries(modelbenchmark PRIVATE Qt6::Test)
target_link_libraries(modelbenchmark PRIVATE model)
add_test(modelbenchmark
	modelbenchmark
	-iterations 8
)
