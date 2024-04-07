
set(TESTS
	testformulafunction
	testmodel
	modelbenchmark
)

add_dependencies(
	build_tests
	${TESTS}
)

# testformulafunction:
add_executable(testformulafunction
	EXCLUDE_FROM_ALL
	testfunction.cpp
	../${SRC_DIR}/model/function.cpp
	../${SRC_DIR}/global.cpp
)
add_test(testformulafunction testformulafunction)
target_include_directories(testformulafunction PRIVATE "${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}")
target_include_directories(testformulafunction PRIVATE "${PROJECT_SOURCE_DIR}/${TEST_DIR}")
target_link_libraries(testformulafunction PRIVATE Qt6::Test)
target_link_libraries(testformulafunction PRIVATE exprtk::exprtk)

set_target_properties(testformulafunction PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# testmodel:
add_executable(testmodel
	EXCLUDE_FROM_ALL
	testmodel.h
	testmodel.cpp
	../${SRC_DIR}/model/model.cpp
	../${SRC_DIR}/model/function.cpp
	../${SRC_DIR}/global.cpp
)

add_test(testmodel testmodel)
target_include_directories(testmodel PRIVATE "${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}")
target_include_directories(testmodel PRIVATE "${PROJECT_SOURCE_DIR}/${TEST_DIR}")
target_link_libraries(testmodel PRIVATE Qt6::Test)
target_link_libraries(testmodel PRIVATE exprtk::exprtk)

set_target_properties(testmodel PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Model Benchmark:
add_executable(modelbenchmark
	EXCLUDE_FROM_ALL
	modelbenchmark.h
	modelbenchmark.cpp
	../${SRC_DIR}/model/model.cpp
	../${SRC_DIR}/model/function.cpp
	../${SRC_DIR}/global.cpp
)
add_test(modelbenchmark modelbenchmark)
target_include_directories(modelbenchmark PRIVATE "${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}")
target_include_directories(modelbenchmark PRIVATE "${PROJECT_SOURCE_DIR}/${TEST_DIR}")
target_link_libraries(modelbenchmark PRIVATE Qt6::Test)
target_link_libraries(modelbenchmark PRIVATE exprtk::exprtk)

set_target_properties(modelbenchmark PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
