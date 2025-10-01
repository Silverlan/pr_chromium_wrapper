include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

set(PROJ_NAME pr_chromium_wrapper)
pr_add_library(${PROJ_NAME} SHARED)

pr_init_module(${PROJ_NAME})

find_package(cef REQUIRED)
pr_add_third_party_dependency(${PROJ_NAME} cef PUBLIC)

if(MSVC)
	target_compile_definitions(${PROJ_NAME} PRIVATE -D_UNICODE -DUNICODE -DNOMINMAX)
	target_compile_options(${PROJ_NAME} PRIVATE "/MT")
endif()

add_subdirectory("subprocess")
pr_add_dependency(pr_chromium_subprocess ${PROJ_NAME} TARGET PUBLIC)

pr_finalize(${PROJ_NAME})
