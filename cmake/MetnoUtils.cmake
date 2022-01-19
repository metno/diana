
MACRO(METNO_CMAKE_SETUP)
  INCLUDE(GNUInstallDirs)
  SET(CMAKE_CXX_STANDARD 11)
ENDMACRO()

#########################################################################

FUNCTION(METNO_GEN_PKGCONFIG pc_in pc_out pc_deps pc_libs_ pc_libdirs_ pc_includedirs_)
  STRING(REPLACE ";" ", " pc_deps "${pc_deps}")
  STRING(REPLACE ">=" " >= " pc_deps "${pc_deps}")

  FOREACH(X ${pc_libs_})
    SET(pc_libs "${pc_libs} -l${X}")
  ENDFOREACH()

  FOREACH(X ${pc_libdirs_})
    IF ((${X} STREQUAL "/usr/lib") OR (${X} STREQUAL "/usr/lib/${CMAKE_INSTALL_LIBDIR}"))
      # skip
    ELSE()
      SET(pc_libdirs "${pc_libdirs} -L${X}")
    ENDIF()
  ENDFOREACH()

  FOREACH(X ${pc_includedirs_})
    IF (${X} STREQUAL "/usr/include")
      # skip
    ELSE()
      SET(pc_includedirs "${pc_includedirs} -I${X}")
    ENDIF()
  ENDFOREACH()

  SET(pc_prefix "${CMAKE_INSTALL_PREFIX}")
  SET(pc_libdir "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
  SET(pc_includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")

  CONFIGURE_FILE(${pc_in} ${pc_out} @ONLY)
  INSTALL(FILES ${CMAKE_BINARY_DIR}/${pc_out} DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig")
ENDFUNCTION()

########################################################################

FUNCTION(METNO_PVERSION_DEFINES pack header_file)
  FILE(STRINGS "${header_file}" version_definitions REGEX "^#define.*_VERSION_(MAJOR|MINOR|PATCH) +")
  FOREACH(v_def ${version_definitions})
    STRING(REGEX REPLACE "^#define.*_VERSION_(MAJOR|MINOR|PATCH) +([0-9]+) *$" \\1 v_type   "${v_def}")
    STRING(REGEX REPLACE "^#define.*_VERSION_(MAJOR|MINOR|PATCH) +([0-9]+) *$" \\2 v_number "${v_def}")
    SET(version_${v_type} "${v_number}")
  ENDFOREACH()

  SET(${pack}_VERSION_MAJOR "${version_MAJOR}" PARENT_SCOPE)
  SET(${pack}_VERSION_MINOR "${version_MINOR}" PARENT_SCOPE)
  SET(${pack}_VERSION_PATCH "${version_PATCH}" PARENT_SCOPE)

  SET(${pack}_PVERSION "${version_MAJOR}.${version_MINOR}" PARENT_SCOPE)
  SET(${pack}_PVERSION_FULL "${version_MAJOR}.${version_MINOR}.${version_PATCH}" PARENT_SCOPE)
ENDFUNCTION()

########################################################################

FUNCTION(METNO_HEADERS headers source_suffix header_suffix)
  FOREACH (_src ${ARGN})
    STRING(REGEX REPLACE "\\.${source_suffix}\$" ".${header_suffix}" _hdr ${_src})
    LIST(APPEND _hdrs ${_hdr})
  ENDFOREACH ()
  SET (${headers} ${_hdrs} PARENT_SCOPE)

  UNSET(_hdrs)
  UNSET(_hdr)
  UNSET(_src)
ENDFUNCTION ()
