add_executable(${TARGET_NAME}
	main.cpp
	controller.cpp
	controller.h
)

set_target_properties(${TARGET_NAME} PROPERTIES
	AUTOMOC ON
)

target_link_libraries(${TARGET_NAME} PUBLIC cpp_flags)
target_link_libraries(${TARGET_NAME} PUBLIC
	audio
	view
	model
	shared
)

