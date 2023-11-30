# TODO: eventually figure out how to get the URL download strategy working on all platforms, it significantly reduces download bandwidth, but breaks MSYS at present

FetchContent_Declare(
        frugally-deep
        #        DOWNLOAD_EXTRACT_TIMESTAMP
        #        URL https://github.com/Dobiasd/frugally-deep/archive/refs/tags/v0.15.26-p0.tar.gz
        GIT_REPOSITORY git@github.com:Dobiasd/frugally-deep.git
        GIT_TAG v0.15.26-p0
)
FetchContent_Declare(
        FunctionalPlus
        #        DOWNLOAD_EXTRACT_TIMESTAMP
        #        URL https://github.com/Dobiasd/FunctionalPlus/archive/refs/tags/v0.2.20-p0.tar.gz
        GIT_REPOSITORY git@github.com:Dobiasd/FunctionalPlus.git
        GIT_TAG v0.2.20-p0
)
FetchContent_Declare(
        Eigen3
        #        DOWNLOAD_EXTRACT_TIMESTAMP
        #        URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
        GIT_REPOSITORY git@github.com:eigen-mirror/eigen.git
        GIT_TAG 3.4.0
)
FetchContent_Declare(nlohmann_json
        #        DOWNLOAD_EXTRACT_TIMESTAMP
        #        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
        GIT_REPOSITORY git@github.com:nlohmann/json.git
        GIT_TAG        v3.11.2
)

# This bypasses nlohmann_json's configuration where it will only install its targets if it thinks it is the top-level project
# Frugally-deep is unable to build if these targets are not exported.
SET(JSON_Install ON)
LIST(APPEND CMAKE_PREFIX_PATH ${nlohmann_json_BINARY_DIR})

FetchContent_MakeAvailable(nlohmann_json Eigen3 FunctionalPlus frugally-deep)