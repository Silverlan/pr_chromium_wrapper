include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

set(PROJ_NAME pr_chromium_subprocess)
pr_add_executable(${PROJ_NAME})

pr_add_headers(${PROJ_NAME} "include/")
pr_add_sources(${PROJ_NAME} "src/")

if(MSVC)
	target_compile_definitions(${PROJ_NAME} PRIVATE -D_UNICODE -DUNICODE -DNOMINMAX)
	target_compile_options(${PROJ_NAME} PRIVATE "/MT")
endif()

pr_finalize(${PROJ_NAME})
