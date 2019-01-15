cmake_minimum_required(VERSION 2.8.12)

if (GIT_FOUND)
    message("git found: ${GIT_EXECUTABLE} in version     ${GIT_VERSION_STRING}")
endif (GIT_FOUND)

SET(BUILD_FILE "/gldcore/build.h")
SET(COMMAND_SCRIPT "./BuildInfo.sh")
SET(GIT_OUTPUT git_out.tmp)

#EXECUTE_PROCESS(COMMAND bash "-c" "'${GIT_EXECUTABLE}' remote -v | grep \"(fetch)\" | sed -n 's/^.*\\t//; \$s/ .*\$//p'"
EXECUTE_PROCESS(
        COMMAND ${GIT_EXECUTABLE} remote -v
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/CMakeScripts
        OUTPUT_FILE ${GIT_OUTPUT}
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
EXECUTE_PROCESS(
        COMMAND bash "-c" "${COMMAND_SCRIPT} ${GIT_OUTPUT} --remote"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/CMakeScripts
        OUTPUT_VARIABLE BUILD_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("BUILD_DIR: ${BUILD_DIR}")

#EXECUTE_PROCESS(COMMAND bash "-c" "\"${GIT_EXECUTABLE}\" status -s | cut -c1-3 | sort -u | sed 's/ //g' | sed ':a;N;$!ba;s/\\n/ /g'"
EXECUTE_PROCESS(
        COMMAND ${GIT_EXECUTABLE} status -s
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/CMakeScripts
        OUTPUT_FILE ${GIT_OUTPUT}
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
EXECUTE_PROCESS(COMMAND bash "-c" "${COMMAND_SCRIPT} ${GIT_OUTPUT} --status"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/CMakeScripts
        OUTPUT_VARIABLE MODIFY_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
message("MODIFY_STATUS: ${MODIFY_STATUS}")

#EXECUTE_PROCESS(COMMAND bash "-c" "git log --max-count=1 --pretty=format:\"%ad\" --date=raw | sed -ne 's/ [\\+\\-][0-9]\\{4\\}$//p'"
EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} log --max-count=1 --format=%ad --date=raw
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/CMakeScripts
        OUTPUT_FILE ${GIT_OUTPUT}
        OUTPUT_STRIP_TRAILING_WHITESPACE)

EXECUTE_PROCESS(COMMAND bash "-c" "${COMMAND_SCRIPT} ${GIT_OUTPUT} --build"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/CMakeScripts
        OUTPUT_VARIABLE BUILD_NUM
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
MATH(EXPR BUILD_NUM "(${BUILD_NUM}/86400)")
message("BUILD_NUM: ${BUILD_NUM}")

FILE(REMOVE ${GIT_OUTPUT})

IF ("" STREQUAL ${MODIFY_STATUS})
    SET(BRANCH "${GIT_COMMIT_HASH}:${GIT_BRANCH}")
ELSE ()
    SET(BRANCH "${GIT_COMMIT_HASH}:${GIT_BRANCH}:Modified")
ENDIF ()

MESSAGE("Updating ${CMAKE_CURRENT_SOURCE_DIR}${BUILD_FILE}: revision ${BUILD_NUM} (${BRANCH})")
STRING(TIMESTAMP BUILD_YEAR "%Y")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gldcore/build.h.in ${CMAKE_CURRENT_SOURCE_DIR}/gldcore/build.h @ONLY)
