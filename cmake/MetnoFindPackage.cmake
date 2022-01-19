
INCLUDE(FindPkgConfigBugfixLibraryPaths)

FUNCTION(METNO_ADD_IMPORTED_LIBRARY name libraries includes definitions)
  add_library(${name} INTERFACE IMPORTED)
  set_target_properties(
    ${name}
    PROPERTIES
      INTERFACE_LINK_LIBRARIES "${libraries}"
      INTERFACE_INCLUDE_DIRECTORIES "${includes}"
      INTERFACE_COMPILE_DEFINITIIONS "${definitions}"
  )
ENDFUNCTION()

FUNCTION(METNO_FIND_PACKAGE ffp)
  set(ARGS0
    # options
  )
  set(ARGS1
    # arguments with one value
    VERSION_MIN
    CMAKE_NAME
    PKGCONFIG_NAME
    LIBRARY_NAME
  )
  set(ARGSN
    # arguments with multiple values
    CMAKE_TARGETS
    INCLUDE_HDR
  )
  CMAKE_PARSE_ARGUMENTS(_ffp "${ARGS0}" "${ARGS1}" "${ARGSN}" ${ARGN})

  # try to find via cmake
  IF (_ffp_CMAKE_NAME)
    FIND_PACKAGE(${_ffp_CMAKE_NAME} ${_ffp_VERSION_MIN} QUIET)
    IF(${_ffp_CMAKE_NAME}_FOUND)
      IF (NOT(DEFINED _ffp_CMAKE_TARGETS))
        SET(_ffp_CMAKE_TARGET ${_ffp_CMAKE_NAME})
      ENDIF()
      FOREACH(_ffp_target ${_ffp_CMAKE_TARGETS})
        IF(TARGET ${_ffp_target})
          GET_TARGET_PROPERTY(_ffp_is_imported "${_ffp_target}" IMPORTED)
          IF(_ffp_is_imported)
            SET(${ffp}_PACKAGE "${_ffp_target}" PARENT_SCOPE)
            MESSAGE(STATUS "Found ${ffp} via cmake imported")
            RETURN()
          ENDIF()
        ENDIF()
      ENDFOREACH ()

      METNO_ADD_IMPORTED_LIBRARY(${ffp}_IMP
        "${${_ffp_CMAKE_TARGET}_LIBRARIES}"
        "${${_ffp_CMAKE_TARGET}_INCLUDE_DIRS}"
        "${${_ffp_CMAKE_TARGET}_DEFINITIONS}"
      )
      SET(${ffp}_PACKAGE "${ffp}_IMP" PARENT_SCOPE)
      MESSAGE(STATUS "Found ${ffp} via cmake")
      RETURN()
    ENDIF()
  ENDIF()

  # try to find via pkg-config
  IF(_ffp_PKGCONFIG_NAME)
    IF(_ffp_VERSION_MIN)
      SET(_ffp_pc_version "${_ffp_PKGCONFIG_NAME}>=${_ffp_VERSION_MIN}")
    ELSE()
      SET(_ffp_pc_version "${_ffp_PKGCONFIG_NAME}")
    ENDIF()
    SET (_ffp_pc ${_ffp_PKGCONFIG_NAME})
    UNSET(${_ffp_pc}_FOUND CACHE)
    PKG_CHECK_MODULES(${_ffp_pc} IMPORTED_TARGET QUIET "${_ffp_pc_version}")
    IF(${_ffp_pc}_FOUND)
      SET(${ffp}_PACKAGE "PkgConfig::${_ffp_pc}" PARENT_SCOPE)
      MESSAGE(STATUS "Found ${ffp} via pkg-config '${_ffp_pc_version}'")
      RETURN()
    ENDIF()
  ENDIF()

  # try to find via library and header
  UNSET(_ffp_inc_dir CACHE)
  FOREACH(_ffp_inc_hdr ${_ffp_INCLUDE_HDR})
    FIND_PATH(_ffp_inc_dir
      NAMES ${_ffp_inc_hdr}
      HINTS "${${ffp}_INCLUDE_DIR}" "${${ffp}_DIR}/include"
    )
    IF (_ffp_inc_dir)
      SET(${ffp}_HEADER ${_ffp_inc_hdr} PARENT_SCOPE)
      BREAK()
    ENDIF()
  ENDFOREACH()
  IF (NOT _ffp_inc_dir)
    MESSAGE(FATAL_ERROR "Cannot find ${ffp}, include header '${_ffp_INCLUDE_HDR}' not found")
  ENDIF()
  IF (_ffp_LIBRARY_NAME)
    UNSET(_ffp_lib CACHE)
    FIND_LIBRARY(_ffp_lib
      NAMES ${_ffp_LIBRARY_NAME}
      HINTS "${${ffp}_LIB_DIR}" "${${ffp}_DIR}/lib"
    )
    IF (NOT _ffp_lib)
      MESSAGE(FATAL_ERROR "Cannot find ${ffp}, library '${_ffp_LIBRARY_NAME}' not found")
    ENDIF()
  ENDIF()
  METNO_ADD_IMPORTED_LIBRARY(${ffp}_LIB_INC "${_ffp_lib}" "${_ffp_inc_dir}" "")
  SET(${ffp}_PACKAGE "${ffp}_LIB_INC" PARENT_SCOPE)
  MESSAGE(STATUS "Found ${ffp} via find_package/find_path, lib='${_ffp_lib}', include='${_ffp_inc_dir}'")
ENDFUNCTION()
