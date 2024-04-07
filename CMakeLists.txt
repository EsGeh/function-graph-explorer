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
	${SRC_DIR}/model/model.cpp
	${SRC_DIR}/model/function.cpp
	${SRC_DIR}/view/mainwindow.cpp
	${SRC_DIR}/view/functionview.cpp
	${SRC_DIR}/view/graphview.cpp
	${SRC_DIR}/view/functiondisplayoptions.cpp
	${SRC_DIR}/view/jack.cpp
	${SRC_DIR}/controller.cpp
	${SRC_DIR}/global.cpp
	${INCLUDE_DIR}/model/model.h
	${INCLUDE_DIR}/model/function.h
	${INCLUDE_DIR}/view/mainwindow.h
	${INCLUDE_DIR}/view/functionview.h
	${INCLUDE_DIR}/view/graphview.h
	${INCLUDE_DIR}/view/functiondisplayoptions.h
	${INCLUDE_DIR}/view/jack.h
	${INCLUDE_DIR}/view/viewdata.h
	${INCLUDE_DIR}/complex.h
	${INCLUDE_DIR}/complex_adaptor.h
	${INCLUDE_DIR}/controller.h
	${INCLUDE_DIR}/global.h
	${UI_DIR}/mainwindow.ui
	${UI_DIR}/functionview.ui
	${UI_DIR}/functiondisplayoptions.ui
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ui/)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

configure_file(
	include/config.h.in
	${INCLUDE_DIR}/config.h
	@ONLY
)

target_include_directories(${TARGET_NAME} PUBLIC
	"${PROJECT_BINARY_DIR}/include"
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

add_custom_target(CopyCompileCommands
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/compile_commands.json ${PROJECT_SOURCE_DIR}/compile_commands.json
)

# TESTS:

add_custom_target(build_tests)

enable_testing(true)
include(CTest)

add_subdirectory(tests)
