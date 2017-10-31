
MESSAGE (STATUS "Generating ${BUILDINFO_CC}")
SET (BUILDINFO_TMP "${BUILDINFO_CC}.tmp")

SET (GA "${GIT_SOURCE_DIR}/.git_archival.txt")
IF (EXISTS "${GA}")
  ### read commit info and timestamp from file

  FILE (STRINGS "${GA}" git_archival)
  LIST (LENGTH git_archival git_archival_length)
  IF (git_archival_length EQUAL 2)
    ### format: 1st line: commit info, 2nd line: timestamp
    LIST (GET git_archival 0 GIT_COMMIT)
    LIST (GET git_archival 1 BUILD_TIMESTAMP)
  ELSEIF (git_archival_length EQUAL 1)
    ### format: up to first space: commit info, remainder: timestamp
    LIST (GET git_archival 0 git_archival_1)
    STRING (REGEX REPLACE "^([^ ]+) +(.*)$" "\\1" GIT_COMMIT      "${git_archival_1}")
    STRING (REGEX REPLACE "^([^ ]+) +(.*)$" "\\2" BUILD_TIMESTAMP "${git_archival_1}")
    UNSET (git_archival_1)
  ELSE ()
    MESSAGE (FATAL_ERROR "Cannot parse '${GA}' contents.")
  ENDIF ()
  UNSET (git_archival)
  UNSET (git_archival_length)
  MESSAGE (STATUS "Read commit '${GIT_COMMIT}' and timestamp '${BUILD_TIMESTAMP}' from file '${GA}'.")

ELSE ()

  ### build commit info
  FIND_PACKAGE(Git QUIET)
  IF (GIT_FOUND)
    EXECUTE_PROCESS(
      COMMAND git describe --abbrev=40 --dirty --always
      WORKING_DIRECTORY "${GIT_SOURCE_DIR}"
      RESULT_VARIABLE git_exit
      OUTPUT_VARIABLE git_output
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    IF (NOT git_exit EQUAL 0)
      SET (git_output "git-error")
    ENDIF ()
    SET(GIT_COMMIT ${git_output})
  ELSE ()
    SET(GIT_COMMIT "git-not-found")
  ENDIF ()

  ### build date
  STRING (TIMESTAMP BUILD_TIMESTAMP
    #"%Y-%m-%dT%H:%M" # build day + hour + minute
    "%Y-%m-%d" # only build day
    UTC)

ENDIF ()

FILE (WRITE ${BUILDINFO_TMP} "#include <diBuild.h>\nconst char diana_build_string[] = \"${BUILD_TIMESTAMP}\";\nconst char diana_build_commit[] = \"${GIT_COMMIT}\";\n")

EXECUTE_PROCESS(
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUILDINFO_TMP} ${BUILDINFO_CC}
)
