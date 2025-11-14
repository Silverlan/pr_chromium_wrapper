include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

set(PROJ_NAME pr_chromium_wrapper)
pr_add_library(${PROJ_NAME} SHARED)

pr_init_module(${PROJ_NAME})

find_package(cef REQUIRED)
pr_add_third_party_dependency(${PROJ_NAME} cef PUBLIC)

if(MSVC)
	target_compile_definitions(${PROJ_NAME} PRIVATE -D_UNICODE -DUNICODE -DNOMINMAX)
	target_compile_options(${PROJ_NAME} PRIVATE "/MT")

	# "import std" currently does not work with /MT
	set_target_properties(${PROJ_NAME} PROPERTIES CXX_MODULE_STD OFF)
endif()

add_subdirectory("subprocess")

pr_finalize(${PROJ_NAME})
