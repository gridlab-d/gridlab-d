# Version using HTTPS:
# FetchContent_Declare(
#   frugally-deep
#   GIT_REPOSITORY https://github.com/Dobiasd/frugally-deep.git
#   GIT_TAG        v0.15.26-p0
# )
# FetchContent_Declare(
#   FunctionalPlus
#   GIT_REPOSITORY https://github.com/Dobiasd/FunctionalPlus.git
#   GIT_TAG        v0.2.20-p0
# )
# FetchContent_Declare(
#   Eigen3
#   GIT_REPOSITORY https://github.com/eigen-mirror/eigen.git
#   GIT_TAG        3.4.0
# )
# FetchContent_Declare(
#   nlohmann_json
#   GIT_REPOSITORY https://github.com/nlohmann/json.git
#   GIT_TAG        v3.11.2
# )

FetchContent_Declare(
  frugally-deep
  GIT_REPOSITORY git@github.com:Dobiasd/frugally-deep.git
  GIT_TAG        v0.15.26-p0
)
FetchContent_Declare(
  FunctionalPlus
  GIT_REPOSITORY git@github.com:Dobiasd/FunctionalPlus.git
  GIT_TAG        v0.2.20-p0
)
FetchContent_Declare(
  Eigen3
  GIT_REPOSITORY git@github.com:eigen-mirror/eigen.git
  GIT_TAG        3.4.0
)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY git@github.com:nlohmann/json.git
  GIT_TAG        v3.11.2
)

FetchContent_MakeAvailable(frugally-deep FunctionalPlus Eigen3 nlohmann_json)