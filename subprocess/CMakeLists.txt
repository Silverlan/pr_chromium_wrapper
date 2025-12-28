include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

set(PROJ_NAME pr_chromium_subprocess)
pr_add_executable(${PROJ_NAME})

pr_init_module(${PROJ_NAME})

if(WIN32)
	target_compile_definitions(${PROJ_NAME} PRIVATE -D_UNICODE -DUNICODE -DNOMINMAX)
	set_property(TARGET ${PROJ_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

pr_add_dependency(${PROJ_NAME} pr_chromium_wrapper TARGET PUBLIC)

pr_finalize(${PROJ_NAME})
