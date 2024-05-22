cmake_minimum_required(VERSION 3.29.1)
project(calculator)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(GMODULE_EXPORT REQUIRED IMPORTED_TARGET gmodule-export-2.0)

find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/calculator.gresource.c
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMAND
		${GLIB_COMPILE_RESOURCES} ARGS resource/calculator.gresource.xml
		--target=${CMAKE_CURRENT_BINARY_DIR}/calculator.gresource.c
		--sourcedir=resource/calculator --generate-source
	VERBATIM
	MAIN_DEPENDENCY resource/calculator.gresource.xml
	DEPENDS resource/calculator/window.ui resource/calculator/style.css
			resource/calculator/icons/scalable/apps/calculator.svg
)

add_custom_target(
	calculator_resource
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/calculator.gresource.c
)

add_executable(
	${PROJECT_NAME}
	${CMAKE_CURRENT_BINARY_DIR}/calculator.gresource.c
	source/calculator/evaluate.c source/calculator/application.c source/main.c
)
target_include_directories(${PROJECT_NAME} PRIVATE header)
target_link_directories(${PROJECT_NAME} PRIVATE library)
target_link_libraries(
	${PROJECT_NAME} PRIVATE PkgConfig::GTK4 PkgConfig::GMODULE_EXPORT
)
target_compile_options(
	${PROJECT_NAME}
	PRIVATE -O2
			-Werror
			-Wall
			-Wextra
			-pedantic
			-Wfloat-equal
			-Wundef
			-Wshadow
			-Wpointer-arith
			-Wcast-align
			-Wstrict-prototypes
			-Wstrict-overflow=5
			-Wwrite-strings
			-Waggregate-return
			-Wcast-qual
			-Wswitch-default
			-Wswitch-enum
			-Wconversion
			-Wunreachable-code
			-Wno-overlength-strings
)
