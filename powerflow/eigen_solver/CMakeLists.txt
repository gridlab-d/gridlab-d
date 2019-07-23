    cmake_minimum_required(VERSION 3.5)
    project(solver_EIGEN)

#    include_directories(${CMAKE_CURRENT_LIST_DIR})
    #SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I../third_party/SuperLU_MT -I../third_party/jsonCpp" )

    add_library(${PROJECT_NAME} SHARED
            solver_EIGEN.cpp
            solver_EIGEN.h
            )

    set_target_properties(${PROJECT_NAME}
            PROPERTIES PREFIX ""
            )

    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    #target_link_libraries(${PROJECT_NAME} libjsoncpp libsuperlu)

    #target_link_libraries(${PROJECT_NAME} ${OS_SPECIFIC_LIBRARIES})
