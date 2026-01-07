cmake_minimum_required(VERSION 4.2)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

set(PROJ_NAME pr_chromium_wrapper)
project(${PROJ_NAME} CXX)
add_library(${PROJ_NAME} SHARED)

if(UNIX)
	set_target_properties(${PROJ_NAME} PROPERTIES
		BUILD_WITH_INSTALL_RPATH TRUE
	)

	set_target_properties(${PROJ_NAME} PROPERTIES
		INSTALL_RPATH "$ORIGIN:$ORIGIN/lib"
	)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	if(NOT WIN32)
		# Required for "import std;" with clang
		target_compile_options(${PROJ_NAME} PUBLIC -stdlib=libstdc++)
		target_link_options(${PROJ_NAME} PUBLIC -stdlib=libstdc++)
	endif()
endif()

target_compile_features(${PROJ_NAME} PRIVATE cxx_std_23)
set_target_properties(${PROJ_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJ_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE SOURCE_LIST "src/*.cpp")
target_sources(${PROJ_NAME} PRIVATE ${SOURCE_LIST})

file(GLOB_RECURSE MODULE_LIST "src/*.cppm")
target_sources(${PROJ_NAME} PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES ${MODULE_LIST})

find_package(cef REQUIRED)

target_include_directories(${PROJ_NAME} PUBLIC ${cef_INCLUDE_DIRS})
target_link_libraries(${PROJ_NAME} PUBLIC ${cef_LIBRARIES})

if(WIN32)
	target_compile_definitions(${PROJ_NAME} PRIVATE -D_UNICODE -DUNICODE -DNOMINMAX)
	set_property(TARGET ${PROJ_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")

	# "import std" currently does not work with /MT
	set_target_properties(${PROJ_NAME} PROPERTIES CXX_MODULE_STD OFF)
endif()

add_subdirectory("subprocess")
