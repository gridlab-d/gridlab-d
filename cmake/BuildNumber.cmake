SET(GIT_OUTPUT git_out_${CMAKE_BUILD_TYPE})

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build_number)

EXECUTE_PROCESS(
        COMMAND ${GIT_EXECUTABLE} remote -v
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build_number
        OUTPUT_FILE ${GIT_OUTPUT}_remote.tmp
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
EXECUTE_PROCESS(
        COMMAND ${GIT_EXECUTABLE} status -s
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build_number
        OUTPUT_FILE ${GIT_OUTPUT}_status.tmp
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} log --max-count=1 --format=%ad --date=raw
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build_number
        OUTPUT_FILE ${GIT_OUTPUT}_build.tmp
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

IF (WIN32)
    SET(COMMAND_SCRIPT ".\\BuildInfo.ps1")
    MESSAGE(STATUS "Using Powershell to detect build data.")
    EXECUTE_PROCESS(
            COMMAND powershell -noprofile -ExecutionPolicy Bypass -nologo -file ${COMMAND_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/build_number/${GIT_OUTPUT}_remote.tmp --remote
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
            OUTPUT_VARIABLE BUILD_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    #    MESSAGE(STATUS "BUILD_DIR: ${BUILD_DIR}")

    EXECUTE_PROCESS(
            COMMAND powershell -noprofile -ExecutionPolicy Bypass -nologo -file ${COMMAND_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/build_number/${GIT_OUTPUT}_status.tmp --status
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
            OUTPUT_VARIABLE MODIFY_STATUS
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    #    MESSAGE(STATUS "MODIFY_STATUS: ${MODIFY_STATUS}")

    EXECUTE_PROCESS(
            COMMAND powershell -noprofile -ExecutionPolicy Bypass -nologo -file ${COMMAND_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/build_number/${GIT_OUTPUT}_build.tmp --build
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
            OUTPUT_VARIABLE BUILD_NUM
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (BUILD_NUM MATCHES "^[0-9]+$")
        MATH(EXPR BUILD_NUM "(${BUILD_NUM}/86400)")
    endif ()
    #    MESSAGE(STATUS "BUILD_NUM: ${BUILD_NUM}")
ELSE ()
    SET(COMMAND_SCRIPT "./BuildInfo.sh")
    #    MESSAGE(STATUS "Using bash to detect build data.")
    EXECUTE_PROCESS(
            COMMAND bash "-c" "${COMMAND_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/build_number/${GIT_OUTPUT}_remote.tmp --remote"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
            OUTPUT_VARIABLE BUILD_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
#    MESSAGE(STATUS "BUILD_DIR: ${BUILD_DIR}")

    EXECUTE_PROCESS(COMMAND bash "-c" "${COMMAND_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/build_number/${GIT_OUTPUT}_status.tmp --status"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
            OUTPUT_VARIABLE MODIFY_STATUS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
#    MESSAGE(STATUS "MODIFY_STATUS: ${MODIFY_STATUS}")

    EXECUTE_PROCESS(COMMAND bash "-c" "${COMMAND_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/build_number/${GIT_OUTPUT}_build.tmp --build"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
            OUTPUT_VARIABLE BUILD_NUM
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
    if (BUILD_NUM MATCHES "^[0-9]+$")
        MATH(EXPR BUILD_NUM "(${BUILD_NUM}/86400)")
    endif ()
#    MESSAGE(STATUS "BUILD_NUM: ${BUILD_NUM}")
ENDIF (WIN32)

IF ("" STREQUAL "${MODIFY_STATUS}" OR "??" STREQUAL "${MODIFY_STATUS}")
    SET(BRANCH "${GIT_COMMIT_HASH}:${GIT_BRANCH}")
ELSE ()
    SET(BRANCH "${GIT_COMMIT_HASH}:${GIT_BRANCH}:Modified")
ENDIF ()

MESSAGE(STATUS "Generating build.h: revision ${BUILD_NUM} (${BRANCH})")
STRING(TIMESTAMP BUILD_YEAR "%Y")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gldcore/build.h.in ${CMAKE_CURRENT_BINARY_DIR}/headers/build.h @ONLY)
