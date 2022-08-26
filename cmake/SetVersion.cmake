cmake_minimum_required(VERSION 2.8.12)

find_package(Git)

# Get the current working branch
execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash of the working branch
execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Package information for Gridlab-d
SET(DLEXT ${CMAKE_SHARED_LIBRARY_SUFFIX})
SET(PACKAGE "${PROJECT_NAME}")
SET(PACKAGE_BUGREPORT "gridlabd@pnnl.gov")
SET(PACKAGE_NAME "GridLAB-D")
SET(REV_MAJOR 5)
SET(REV_MINOR 0)
SET(REV_PATCH 0)
SET(PACKAGE_STRING "GridLAB-D ${REV_MAJOR}.${REV_MINOR}.${REV_PATCH} (Branch:${GIT_BRANCH} - ${GIT_COMMIT_HASH})")
SET(PACKAGE_TARNAME "${PROJECT_NAME}")
SET(PACKAGE_URL "")
SET(PACKAGE_VERSION ${REV_MAJOR}.${REV_MINOR}.${REV_PATCH})