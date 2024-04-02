cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME fge)
set(INCLUDE_DIR include)
set(SRC_DIR src)
set(UI_DIR ui)
set(TEST_DIR tests)

project(${TARGET_NAME}
    VERSION 0.1
    LANGUAGES CXX
)

set(PROJECT_SOURCES
	${SRC_DIR}/main.cpp
	${SRC_DIR}/mainwindow.cpp
	${SRC_DIR}/functionview.cpp
	${SRC_DIR}/model.cpp
	${SRC_DIR}/function.cpp
	${SRC_DIR}/controller.cpp
	${SRC_DIR}/graphview.cpp
	${SRC_DIR}/functiondisplayoptions.cpp
	${SRC_DIR}/jack.cpp
	${INCLUDE_DIR}/viewdata.h
	${INCLUDE_DIR}/complex.h
	${INCLUDE_DIR}/complex_adaptor.h
	${INCLUDE_DIR}/global.h
	${INCLUDE_DIR}/mainwindow.h
	${INCLUDE_DIR}/functionview.h
	${INCLUDE_DIR}/model.h
	${INCLUDE_DIR}/function.h
	${INCLUDE_DIR}/controller.h
	${INCLUDE_DIR}/graphview.h
	${INCLUDE_DIR}/functiondisplayoptions.h
	${INCLUDE_DIR}/jack.h
	${UI_DIR}/mainwindow.ui
	${UI_DIR}/functionview.ui
	${UI_DIR}/functiondisplayoptions.ui
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ui/)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Find package modules
include (FindPkgConfig)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Charts)
find_package(Qt6 REQUIRED COMPONENTS Test)

find_package(exprtk REQUIRED)

find_package(Jack REQUIRED)

add_executable(${TARGET_NAME} ${PROJECT_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
	"${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}"
)

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Werror)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Widgets)
target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Charts)
target_link_libraries(${TARGET_NAME} PRIVATE exprtk::exprtk)
target_link_libraries(${TARGET_NAME} PRIVATE Jack::Jack)

set_target_properties(${TARGET_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# TESTS:

enable_testing(true)
include(CTest)

# testformulafunction:
add_executable(testformulafunction
	${TEST_DIR}/testfunction.cpp
	${SRC_DIR}/function.cpp
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
	${TEST_DIR}/testmodel.h
	${TEST_DIR}/testmodel.cpp
	${SRC_DIR}/model.cpp
	${SRC_DIR}/function.cpp
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
