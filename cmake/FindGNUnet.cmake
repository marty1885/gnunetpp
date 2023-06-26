if (TARGET GNUnet::GNUnet)
    return()
endif ()

find_path(GNUnet_INCLUDE_DIRS NAMES gnunet/gnunet_common.h
    DOC "The GNUnet include directory")

set(GNUnet_LIBS namestore gnsrecord identity core util fs dht nse cadet peerinfo datastore gns messenger
    transport hello)
set(all_libs_found TRUE)
foreach(lib ${GNUnet_LIBS})
    find_library(GNUnet_${lib}_LIBRARY NAMES gnunet${lib})
    if (NOT GNUnet_${lib}_LIBRARY)
        message(STATUS "Missing GNUnet library: ${lib}")
        set(all_libs_found FALSE)
    endif ()
    list(APPEND GNUnet_LIBRARIES ${GNUnet_${lib}_LIBRARY})
endforeach()

if (NOT all_libs_found)
    message(STATUS "Missing some or all GNUnet library")
    set(GNUnet_FOUND FALSE)
    return()
endif ()

# HACK: CMake requires a library as the "target" of an imported library, so we
# use a library we know will be present on the system.
add_library(GNUnet::GNUnet IMPORTED UNKNOWN)
set_target_properties(GNUnet::GNUnet PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GNUnet_INCLUDE_DIRS}"
    IMPORTED_LOCATION ${GNUnet_util_LIBRARY}
    INTERFACE_LINK_LIBRARIES "${GNUnet_LIBRARIES}"
)

mark_as_advanced(GNUnet_INCLUDE_DIRS GNUnet_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    GNUnet
    REQUIRED_VARS GNUnet_LIBRARIES GNUnet_INCLUDE_DIRS
)