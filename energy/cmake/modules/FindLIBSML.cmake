find_library(LIBSML_LIBRARY
    NAMES
        sml
    DOC "libsml")
find_path(LIBSML_INCLUDE_DIR
    NAMES
        sml/sml_file.h
    DOC "libsml")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBSML DEFAULT_MSG
    LIBSML_LIBRARY LIBSML_INCLUDE_DIR)

mark_as_advanced(
    LIBSML_LIBRARY
    LIBSML_INCLUDE_DIR)

if(LIBSML_FOUND)
    set(LIBSML_LIBRARIES ${LIBSML_LIBRARY})
    set(LIBSML_INCLUDE_DIRS ${LIBSML_INCLUDE_DIR})
endif()
