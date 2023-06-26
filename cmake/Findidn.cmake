if(TARGET idn::idn)
    return()
endif ()

find_library(IDN_LIBRARY NAMES idn)
if(NOT IDN_LIBRARY)
    set(IDN_FOUND FALSE)
    return()
endif()

find_path(IDN_INCLUDE_DIR NAMES idna.h)
if(NOT IDN_INCLUDE_DIR)
    set(IDN_FOUND FALSE)
    return()
endif()

set(IDN_FOUND TRUE)
set(IDN_LIBRARIES ${IDN_LIBRARY})
set(IDN_INCLUDE_DIRS ${IDN_INCLUDE_DIR})
mark_as_advanced(IDN_INCLUDE_DIRS IDN_LIBRARIES)

add_library(idn::idn IMPORTED UNKNOWN)
set_target_properties(idn::idn PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${IDN_INCLUDE_DIRS}"
    IMPORTED_LOCATION ${IDN_LIBRARIES}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    idn
    REQUIRED_VARS IDN_LIBRARIES IDN_INCLUDE_DIRS
)