#
# build perl module
#

IF (ENABLE_PERL)

FIND_PACKAGE(Perl)
IF ((NOT PERL_FOUND) OR (NOT PERL_EXECUTABLE))
  MESSAGE (FATAL_ERROR "no perl")
ENDIF ()

IF (NOT (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR))
  FILE (COPY "${CMAKE_CURRENT_SOURCE_DIR}/Metno-Bdiana"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
    PATTERN "*.in" EXCLUDE)
ENDIF ()

CONFIGURE_FILE(Metno-Bdiana/lib/Metno/Bdiana.pm.in
  Metno-Bdiana/lib/Metno/Bdiana.pm
  @ONLY
)

CONFIGURE_FILE(Metno-Bdiana/Makefile.PL.in
  Metno-Bdiana/Makefile.PL
  @ONLY
)

SET (PERLMODULE_OUT
  ${CMAKE_CURRENT_BINARY_DIR}/Bdiana.o # on linux / unix
)
ADD_CUSTOM_COMMAND(
  OUTPUT
    ${PERLMODULE_OUT}

  COMMAND
    ${PERL_EXECUTABLE} Makefile.PL
  COMMAND
    make

  WORKING_DIRECTORY
    Metno-Bdiana

  DEPENDS
    libdiana
    Metno-Bdiana/Bdiana.xs
    Metno-Bdiana/Changes
    Metno-Bdiana/MANIFEST
    Metno-Bdiana/Makefile.PL.in
    Metno-Bdiana/README
    Metno-Bdiana/control
    Metno-Bdiana/ppport.h
    Metno-Bdiana/t/Metno-Bdiana.t
    Metno-Bdiana/lib/Metno/Bdiana.pm.in

  COMMENT "Building Metno-Bdiana perl module"
)

ADD_CUSTOM_TARGET(perlmodule ALL
  DEPENDS ${PERLMODULE_OUT}
)

# somehow this seems to work even when using make -C /cmake-build install DESTDIR=abcde
INSTALL (CODE "EXECUTE_PROCESS (COMMAND make install WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}/Metno-Bdiana\")")

ADD_TEST(NAME perl_test
  COMMAND make -C Metno-Bdiana test
)

ENDIF (ENABLE_PERL)
