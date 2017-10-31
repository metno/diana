
# index.html includes diana version number
CONFIGURE_FILE(diana/index.html
  diana/index.html
  @ONLY
)

INSTALL (DIRECTORY diana/
  DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/doc/diana-${PVERSION}
  PATTERN diana/index.html EXCLUDE
)
INSTALL (FILES
  ${CMAKE_CURRENT_BINARY_DIR}/diana/index.html
  DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/doc/diana-${PVERSION}
)

########################################################################

SET(MAN_DIANA "diana.bin-${PVERSION}.1")
CONFIGURE_FILE(diana.1.in "${MAN_DIANA}")

SET(MAN_BDIANA "bdiana-${PVERSION}.1")
CONFIGURE_FILE(bdiana.1.in "${MAN_BDIANA}")

INSTALL(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/${MAN_DIANA}"
  "${CMAKE_CURRENT_BINARY_DIR}/${MAN_BDIANA}"
  DESTINATION ${CMAKE_INSTALL_MANDIR}
)

########################################################################

ADD_SUBDIRECTORY(diana-dev)
