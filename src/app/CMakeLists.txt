set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(${TARGET_NAME}
	main.cpp
	controller.cpp
	controller.h
	resources.cpp
	resources.h
	application.h
	application.cpp
  resources.qrc
)

set_target_properties(${TARGET_NAME} PROPERTIES
	AUTOMOC ON
)

find_package(Qt6 REQUIRED COMPONENTS Concurrent)

target_link_libraries(${TARGET_NAME} PUBLIC cpp_flags)
target_link_libraries(${TARGET_NAME} PUBLIC
	audio
	view
	model
	shared
)

target_link_libraries(${TARGET_NAME} PUBLIC Qt6::Concurrent)
