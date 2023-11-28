FetchContent_Declare(
  frugally-deep
  GIT_REPOSITORY git@github.com:Dobiasd/frugally-deep.git
  GIT_TAG        3bfb17f8266e1854d819de8c0d3be0df50872f13 # v0.15.26-p0
)
FetchContent_Declare(
  FunctionalPlus
  GIT_REPOSITORY git@github.com:Dobiasd/FunctionalPlus.git
  GIT_TAG        42eba573f0cd331476bc907827a08a348fecbd31 # v0.2.20-p0
)
FetchContent_Declare(
  Eigen3
  GIT_REPOSITORY git@github.com:eigen-mirror/eigen.git
  GIT_TAG        3147391d946bb4b6c68edd901f2add6ac1f31f8c # v3.4.0
)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY git@github.com:nlohmann/json.git
  GIT_TAG        bc889afb4c5bf1c0d8ee29ef35eaaf4c8bef8a5d # v3.11.2
)

FetchContent_MakeAvailable(frugally-deep FunctionalPlus Eigen3 nlohmann_json)