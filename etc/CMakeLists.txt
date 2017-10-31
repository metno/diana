
CONFIGURE_FILE(diana/diana.setup.in diana/diana.setup @ONLY)

INSTALL (FILES
  ${CMAKE_CURRENT_BINARY_DIR}/diana/diana.setup
  diana/log4cpp.properties
  DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}/diana/${PVERSION}"
)
