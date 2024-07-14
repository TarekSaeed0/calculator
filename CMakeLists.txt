cmake_minimum_required(VERSION 3.29.1)
project(
	calculator
	VERSION 1.0
	DESCRIPTION "Simple calculator built in c"
	LANGUAGES C
)

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
		${GLIB_COMPILE_RESOURCES} data/calculator.gresource.xml
		--target=${CMAKE_CURRENT_BINARY_DIR}/calculator.gresource.c
		--sourcedir=data/calculator --generate-source
	VERBATIM
	MAIN_DEPENDENCY data/calculator.gresource.xml
	DEPENDS
		data/calculator/window.ui
		data/calculator/style.css
		data/calculator/icons/hicolor/scalable/apps/com.github.TarekSaeed0.calculator.svg
)

add_custom_target(
	calculator_compile_gresources ALL
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/calculator.gresource.c
)

find_program(DESKTOP_FILE_VALIDATE NAMES desktop-file-validate)

if(DESKTOP_FILE_VALIDATE)
	add_custom_command(
		OUTPUT
			${CMAKE_CURRENT_BINARY_DIR}/com.github.TarekSaeed0.calculator.desktop.validated
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND
			${DESKTOP_FILE_VALIDATE}
			data/calculator/applications/com.github.TarekSaeed0.calculator.desktop
		COMMAND
			${CMAKE_COMMAND} -E touch
			${CMAKE_CURRENT_BINARY_DIR}/com.github.TarekSaeed0.calculator.desktop.validated
		VERBATIM
		MAIN_DEPENDENCY
			data/calculator/applications/com.github.TarekSaeed0.calculator.desktop
	)

	add_custom_target(
		calculator_validate_desktop_file ALL
		DEPENDS
			${CMAKE_CURRENT_BINARY_DIR}/com.github.TarekSaeed0.calculator.desktop.validated
	)
endif()

add_executable(
	${PROJECT_NAME}
	${CMAKE_CURRENT_BINARY_DIR}/calculator.gresource.c
	src/calculator/evaluate.c src/calculator/ratio.c
	src/calculator/application.c src/main.c
)
target_include_directories(${PROJECT_NAME} PRIVATE include)
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
			-Wcast-qual
			-Wswitch-enum
			-Wconversion
			-Wunreachable-code
			-Wno-overlength-strings
)

install(TARGETS calculator)
install(DIRECTORY data/calculator/icons data/calculator/applications TYPE DATA)
