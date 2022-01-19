SET(CMAKE_AUTOMOC ON)

INCLUDE(CMakeParseArguments)

# METNO_QT_CREATE_TRANSLATION
#
# Based on QT5_CREATE_TRANSLATION from Qt5LinguistToolsMacros.cmake with some
# changes:
#  * custom target for lupdate
#  * "clean" target does not remove translations
# And some problems:
#  * TSFILES must be given with absolute paths
#  * no way to get hold of TARGET_INCLUDE_DIRECTORIES
#
FUNCTION(METNO_QT_CREATE_TRANSLATION _qm_files)
  SET(options) # empty
  SET(oneValueArgs UPDATE_TARGET)
  SET(multiValueArgs TSFILES UPDATE_SOURCES UPDATE_OPTIONS)
  CMAKE_PARSE_ARGUMENTS(_T "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  IF(_T_UPDATE_SOURCES AND _T_UPDATE_TARGET)
    SET(_lupdate_lst_file "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_T_UPDATE_TARGET}_lst_file")
    FILE(WRITE ${_lupdate_lst_file} "")

    GET_DIRECTORY_PROPERTY(_inc_dirs INCLUDE_DIRECTORIES)
    FOREACH(_inc_dir ${_inc_dirs})
      GET_FILENAME_COMPONENT(_inc_abs "${_inc_dir}" ABSOLUTE)
      FILE(APPEND ${_lupdate_lst_file} "-I${_inc_abs}\n")
    ENDFOREACH()

    FOREACH(_src ${_T_UPDATE_SOURCES})
      GET_FILENAME_COMPONENT(_abs_src ${_src} ABSOLUTE)
      FILE(APPEND ${_lupdate_lst_file} "${_abs_src}\n")
    ENDFOREACH()

    ADD_CUSTOM_TARGET(${_T_UPDATE_TARGET}
      COMMAND ${Qt5_LUPDATE_EXECUTABLE} ${_T_UPDATE_OPTIONS} "@${_lupdate_lst_file}" -ts ${_T_TSFILES}
    )
  ENDIF()

  QT5_ADD_TRANSLATION(${_qm_files} ${_T_TSFILES})
  SET(${_qm_files} ${${_qm_files}} PARENT_SCOPE)
ENDFUNCTION()
