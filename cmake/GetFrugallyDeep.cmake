FetchContent_Declare(
        frugally-deep
        DOWNLOAD_EXTRACT_TIMESTAMP
        URL https://github.com/Dobiasd/frugally-deep/archive/refs/tags/v0.15.26-p0.tar.gz
#        GIT_REPOSITORY git@github.com:Dobiasd/frugally-deep.git
#        GIT_TAG v0.15.26-p0
        FIND_PACKAGE_ARGS CONFIG

)
FetchContent_Declare(
        FunctionalPlus
        DOWNLOAD_EXTRACT_TIMESTAMP
        URL https://github.com/Dobiasd/FunctionalPlus/archive/refs/tags/v0.2.20-p0.tar.gz
#        GIT_REPOSITORY git@github.com:Dobiasd/FunctionalPlus.git
#        GIT_TAG v0.2.20-p0
        FIND_PACKAGE_ARGS CONFIG
)
FetchContent_Declare(
        Eigen3
        DOWNLOAD_EXTRACT_TIMESTAMP
        URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
#        GIT_REPOSITORY git@github.com:eigen-mirror/eigen.git
#        GIT_TAG 3.4.0
        FIND_PACKAGE_ARGS CONFIG NO_MODULE
)
FetchContent_Declare(json
        DOWNLOAD_EXTRACT_TIMESTAMP
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
        FIND_PACKAGE_ARGS CONFIG
)

# This bypasses nlohmann_json's configutation where it will only install its targets if it thinks it is the top-level project
# Frugally-deep is unable to build if these targets are not exported.
SET(JSON_Install ON)
FetchContent_MakeAvailable(json Eigen3 FunctionalPlus frugally-deep)