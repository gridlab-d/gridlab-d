cmake_minimum_required(VERSION 2.8.12)

SET(BUILD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/gldcore/build.h")

EXECUTE_PROCESS(COMMAND bash "-c" "git remote -v | grep \"(fetch)\" | sed -n 's/^.*\\t//; \$s/ .*\$//p'"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE BUILD_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
#message("BUILD_DIR: ${BUILD_DIR}")

EXECUTE_PROCESS(COMMAND bash "-c" "git status -s | cut -c1-3 | sort -u | sed 's/ //g' | sed ':a;N;$!ba;s/\\n/ /g'"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MODIFY_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
#message("MODIFY_STATUS: ${MODIFY_STATUS}")

EXECUTE_PROCESS(COMMAND bash "-c" "git log --max-count=1 --pretty=format:\"%ad\" --date=raw | sed -ne 's/ [\\+\\-][0-9]\\{4\\}$//p'"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE BUILD_NUM
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
MATH(EXPR BUILD_NUM "(${BUILD_NUM}/86400)")
#message("BUILD_NUM: ${BUILD_NUM}")

IF ("" STREQUAL ${MODIFY_STATUS})
    SET(BRANCH "${GIT_COMMIT_HASH}:${GIT_BRANCH}")
ELSE ()
    SET(BRANCH "${GIT_COMMIT_HASH}:${GIT_BRANCH}:Modified")
ENDIF ()

if (EXISTS "${BUILD_FILE}")
    EXECUTE_PROCESS(COMMAND bash "-c" "cat ${BUILD_FILE} | sed -ne 's/^\#define BUILDNUM //p'"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE OLDNUM
            OUTPUT_STRIP_TRAILING_WHITESPACE

            )
    EXECUTE_PROCESS(COMMAND bash "-c" "cat ${BUILD_FILE} | sed -ne 's/^\#define BRANCH //p' | sed -e 's/\\\" //g'"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE OLDBRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
else ()
    SET(OLDNUM "0")
endif ()

#MESSAGE("OLDNUM: ${OLDNUM}")
#MESSAGE("GIT_BRANCH: ${GIT_BRANCH}")
#MESSAGE("OLDBRANCH: ${OLDBRANCH}")
#MESSAGE("BRANCH: ${BRANCH}")
#MESSAGE("GIT_COMMIT_HASH: ${GIT_COMMIT_HASH}")

IF (
((NOT ${OLDNUM}) OR NOT (${BUILD_NUM} STREQUAL ${OLDNUM}))
        OR
((NOT ${OLDBRANCH}) OR NOT (${OLDBRANCH} STREQUAL ${BRANCH}))
)
    MESSAGE("Updating ${BUILD_FILE}: revision ${BUILD_NUM} (${BRANCH})")
    STRING(TIMESTAMP BUILD_YEAR "%Y")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gldcore/build.h.in ${CMAKE_CURRENT_SOURCE_DIR}/gldcore/build.h @ONLY)
ENDIF ()