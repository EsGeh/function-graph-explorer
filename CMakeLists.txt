cmake_minimum_required(VERSION 3.5)

project(qt-test
    VERSION 0.1
    LANGUAGES CXX
)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        model.h model.cpp
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Charts)

find_package(exprtk REQUIRED)

add_executable(qt-test
    ${PROJECT_SOURCES}
    controller.h controller.cpp
)

target_link_libraries(qt-test PRIVATE Qt6::Widgets)
target_link_libraries(qt-test PRIVATE Qt6::Charts)
target_link_libraries(qt-test PRIVATE exprtk::exprtk)

set_target_properties(qt-test PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
