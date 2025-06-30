set(PCK "cef")

if (${PCK}_FOUND)
  return()
endif()

find_path(${PCK}_INCLUDE_DIR
  NAMES include/cef_config.h
  HINTS
    ${PRAGMA_DEPS_DIR}/cef/include
)

find_path(${PCK}_RESOURCE_DIR
  NAMES resources.pak
  HINTS
    ${PRAGMA_DEPS_DIR}/cef/Resources
)

find_library(${PCK}_LIBRARY
  NAMES cef
  HINTS
    ${PRAGMA_DEPS_DIR}/cef/lib
)

find_library(${PCK}_DLL_WRAPPER_LIBRARY
  NAMES cef_dll_wrapper
  HINTS
    ${PRAGMA_DEPS_DIR}/cef/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${PCK}
  REQUIRED_VARS ${PCK}_LIBRARY ${PCK}_DLL_WRAPPER_LIBRARY ${PCK}_INCLUDE_DIR ${PCK}_RESOURCE_DIR
)

if(${PCK}_FOUND)
  set(${PCK}_LIBRARIES   ${${PCK}_LIBRARY} ${${PCK}_DLL_WRAPPER_LIBRARY})
  set(${PCK}_INCLUDE_DIRS ${${PCK}_INCLUDE_DIR})
endif()
