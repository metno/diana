FIND_PACKAGE(GTest QUIET)
IF(GTEST_FOUND)
  IF (CMAKE_VERSION VERSION_LESS "3.20")
    SET(GTEST_LIBRARY GTest::GTest)
    SET(GTEST_MAIN_LIBRARY GTest::Main)
  ELSE()
    SET(GTEST_LIBRARY GTest::gtest)
    SET(GTEST_MAIN_LIBRARY GTest::gtest_main)
  ENDIF()
ELSE()
  MESSAGE("apparently no compiled GTest library, trying to build it")
  FIND_FILE(GTEST_DIR src/gtest-all.cc
    HINTS
    "${GTEST_ROOT}/src/gtest"
    /usr/src/googletest/googletest
    /usr/src/gmock/gtest
    /usr/src/gtest
    /usr/local/src/gtest
  )
  IF(NOT GTEST_DIR)
    MESSAGE(FATAL_ERROR "could not find gtest-all.cc")
  ENDIF()
  GET_FILENAME_COMPONENT(GTEST_DIR ${GTEST_DIR} PATH)
  GET_FILENAME_COMPONENT(GTEST_DIR ${GTEST_DIR} PATH)
  IF(EXISTS "${GTEST_DIR}/../CMakeLists.txt")
    # bionic, focal:
    # - package 'googletest' includes googlemock
    # - cmake complains if not using topmost CMakeLists.txt
    GET_FILENAME_COMPONENT(GTEST_DIR ${GTEST_DIR} PATH)
  ELSE()
    # xenial:
    # - package 'libgtest-dev' does not include googlemock
    # - no extra subdirectory
  ENDIF()

  SET(BUILD_GMOCK OFF CACHE BOOL "do not build gmock" FORCE)
  SET(BUILD_GTEST ON  CACHE BOOL "build gtest" FORCE)
  ADD_SUBDIRECTORY(${GTEST_DIR} ${CMAKE_CURRENT_BINARY_DIR}/gtest EXCLUDE_FROM_ALL)

  SET(GTEST_LIBRARY gtest)
  SET(GTEST_MAIN_LIBRARY gtest_main)
ENDIF()
