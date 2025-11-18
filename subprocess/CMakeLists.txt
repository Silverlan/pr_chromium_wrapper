include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

set(PROJ_NAME pr_chromium_subprocess)
pr_add_executable(${PROJ_NAME})

pr_init_module(${PROJ_NAME})

if(MSVC)
	target_compile_definitions(${PROJ_NAME} PRIVATE -D_UNICODE -DUNICODE -DNOMINMAX)
	target_compile_options(${PROJ_NAME} PRIVATE "/MT")
endif()

pr_add_dependency(${PROJ_NAME} pr_chromium_wrapper TARGET PUBLIC)

pr_finalize(${PROJ_NAME})
