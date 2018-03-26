SET(METNO_QT_VERSION "5" CACHE STRING "Choose 4 for Qt4 or 5 for Qt5")
SET(CMAKE_AUTOMOC ON)

INCLUDE(CMakeParseArguments)

MACRO(METNO_FIND_QT components)
  SET(BUILDONLY FALSE)
  IF(METNO_QT_VERSION MATCHES 4)
    FOREACH(c ${ARGV})
      IF(${c} STREQUAL BUILDONLY)
        SET(BUILDONLY TRUE)
      ELSEIF(NOT "${c}" STREQUAL "PrintSupport")
        IF("${c}" STREQUAL "Widgets")
          SET(qt_comp "QtGui")
        ELSE()
          SET(qt_comp "Qt${c}")
        ENDIF()
        LIST(APPEND qt_components ${qt_comp})
        LIST(APPEND qt_libs "Qt4::${qt_comp}")
        IF (NOT BUILDONLY)
          LIST(APPEND METNO_PC_DEPS_QT ${qt_comp})
        ENDIF ()
      ENDIF ()
    ENDFOREACH()
    SET(METNO_QT_SUFFIX "")
  ELSEIF(METNO_QT_VERSION MATCHES 5)
    SET(METNO_QT_SUFFIX "-qt5")
    FOREACH(c ${ARGV})
      IF (${c} STREQUAL BUILDONLY)
        SET(BUILDONLY TRUE)
      ELSE ()
        LIST(APPEND qt_components ${c})
        LIST(APPEND qt_libs "Qt5::${c}")
        IF (NOT BUILDONLY)
          LIST(APPEND METNO_PC_DEPS_QT "Qt5${c}")
        ENDIF ()
      ENDIF ()
    ENDFOREACH()
    LIST(APPEND qt_components LinguistTools)
  ELSE()
    MESSAGE(SEND_ERROR "METNO_QT_VERSION should be 4 or 5")
  ENDIF()
  FIND_PACKAGE(Qt${METNO_QT_VERSION} REQUIRED COMPONENTS ${qt_components})
  SET(QT_LIBRARIES ${qt_libs})
ENDMACRO()

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
  IF(NOT (METNO_QT_VERSION MATCHES 5))
    MESSAGE(SEND_ERROR "METNO_QT_CREATE_TRANSLATION only implemented for Qt5")
  ENDIF()

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
