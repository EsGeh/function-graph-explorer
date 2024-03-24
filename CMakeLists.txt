cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME fge)
set(INCLUDE_DIR include)
set(SRC_DIR src)
set(UI_DIR ui)

project(${TARGET_NAME}
    VERSION 0.1
    LANGUAGES CXX
)

set(PROJECT_SOURCES
	${SRC_DIR}/main.cpp
	${SRC_DIR}/mainwindow.cpp
	${SRC_DIR}/functionview.cpp
	${SRC_DIR}/model.cpp
	${SRC_DIR}/controller.cpp
	${SRC_DIR}/graphview.cpp
	${INCLUDE_DIR}/mainwindow.h
	${INCLUDE_DIR}/functionview.h
	${INCLUDE_DIR}/model.h
	${INCLUDE_DIR}/controller.h
	${INCLUDE_DIR}/graphview.h
	${UI_DIR}/mainwindow.ui
	${UI_DIR}/functionview.ui
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ui/)
set(AUTOMOC_PATH_PREFIX ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Charts)

find_package(exprtk REQUIRED)

add_executable(${TARGET_NAME} ${PROJECT_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}")

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Werror)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Widgets)
target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Charts)
target_link_libraries(${TARGET_NAME} PRIVATE exprtk::exprtk)

set_target_properties(${TARGET_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
