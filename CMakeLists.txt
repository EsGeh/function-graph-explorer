cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME fge)
set(SRC_DIR src)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${TARGET_NAME}
    VERSION 0.1
    LANGUAGES CXX
)

add_library(cpp_flags INTERFACE)
target_compile_features(cpp_flags INTERFACE cxx_std_23)
if(MSVC)
	target_compile_options(cpp_flags INTERFACE /W4 /WX)
else()
  target_compile_options(cpp_flags INTERFACE -Wall -Werror)
endif()

add_subdirectory(${SRC_DIR}/app)
add_subdirectory(${SRC_DIR}/shared)
add_subdirectory(${SRC_DIR}/model)
add_subdirectory(${SRC_DIR}/view)
add_subdirectory(${SRC_DIR}/audio)
add_subdirectory(tests)

add_custom_target(CopyCompileCommands
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/compile_commands.json ${PROJECT_SOURCE_DIR}/compile_commands.json
)
