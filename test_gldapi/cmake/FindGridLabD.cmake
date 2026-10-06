# FindGridLABD.cmake

# Try to locate the GridLAB-D installation
# You need to set GridLABD_DIR manually vid -DGridLABD_DIR

# Require GridLABD_DIR to be set
if(NOT DEFINED GridLABD_DIR)
    message(FATAL_ERROR
        "❌ GridLABD_DIR is not set. Please set it to the GridLAB-D installation directory using:\n"
        "   -DGridLABD_DIR=/path/to/gridlabd"
    )
endif()

find_path(GridLABD_INCLUDE_DIR
    NAMES gldapi.h
    PATHS
        ${GridLABD_DIR}/include
)

find_library(GridLABD_LIBRARY
    NAMES gldapi
    PATHS
        ${GridLABD_DIR}/lib
)

# Set the standard output variables
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GridLABD
    REQUIRED_VARS GridLABD_LIBRARY GridLABD_INCLUDE_DIR
    HANDLE_COMPONENTS
)

# Export variables
if(GridLABD_FOUND)
    set(GridLABD_LIBRARIES ${GridLABD_LIBRARY})
    set(GridLABD_INCLUDE_DIRS ${GridLABD_INCLUDE_DIR})
endif()
