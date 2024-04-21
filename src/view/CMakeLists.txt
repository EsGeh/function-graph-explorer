set(UI_DIR ui)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${UI_DIR})

qt_add_library(view
	mainwindow.cpp
	functionview.cpp
	graphview.cpp
	functiondisplayoptions.cpp
	include/fge/view/mainwindow.h
	include/fge/view/functionview.h
	include/fge/view/graphview.h
	include/fge/view/functiondisplayoptions.h
	${UI_DIR}/mainwindow.ui
	${UI_DIR}/functionview.ui
	${UI_DIR}/functiondisplayoptions.ui
)

target_link_libraries(view PUBLIC cpp_flags)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Charts)


target_include_directories(view PUBLIC include)

target_link_libraries(view PUBLIC shared)
target_link_libraries(view PUBLIC Qt6::Charts)
target_link_libraries(view PUBLIC Qt6::Widgets)
target_link_libraries(view PUBLIC Qt6::Charts)
