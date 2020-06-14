#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libember::ember-shared" for configuration ""
set_property(TARGET libember::ember-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(libember::ember-shared PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libember-shared.dylib"
  IMPORTED_SONAME_NOCONFIG "@rpath/libember-shared.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS libember::ember-shared )
list(APPEND _IMPORT_CHECK_FILES_FOR_libember::ember-shared "${_IMPORT_PREFIX}/lib/libember-shared.dylib" )

# Import target "libember::ember-static" for configuration ""
set_property(TARGET libember::ember-static APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(libember::ember-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libember-static.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS libember::ember-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_libember::ember-static "${_IMPORT_PREFIX}/lib/libember-static.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
