# -*- autoconf -*-
#
# $Id: metno_program_suffix.m4 212 2010-04-09 09:32:02Z dages $
#
# SYNOPSIS
#
#   METNO_PROGRAM_SUFFIX

# DESCRIPTION
#
#   The METNO_PROGRAM_SUFFIX macro is a met.no in-house funktion to
#   make the met.no installation environment workable.
#   METNO_PROGRAM_SUFFIX is triggered under ./configure --program-suffix
#   and defines the ENABLE_BETA Make condition and the BETA substitution
#   used by make install and make debian. One can distribute met.no software
#   into different directories or debian packages - dependant on the
#   suffix name. The usual suffix at met.no is -TEST, but one can use any
#   name and change the setpup system according to this

AC_DEFUN([METNO_PROGRAM_SUFFIX], [
    AM_CONDITIONAL([HAS_PROGRAM_SUFFIX], [test x"$program_suffix" != "xNONE"])
    AS_IF([test x"$program_suffix" != "xNONE"], [
	APP="$PACKAGE_NAME$program_suffix"
	AC_SUBST([DEBIAN_INPUT_BASE], [$APP])
	AC_SUBST([PSUFFIX],[$program_suffix])
	AC_SUBST([APP],[$APP])
    ], [
	AC_SUBST([APP],[$PACKAGE_NAME])
    ])
    AC_SUBST([program_suffix])
])
