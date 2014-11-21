# -*- autoconf -*-
#
# $Id: metno_ftgl.m4 186 2010-01-19 19:38:07Z dages $
#
# SYNOPSIS
#
#   METNO_REQUIRE_FTGL
#
# DESCRIPTION
#
#   The METNO_REQUIRE_FTGL macro searches for the FTGL library
#   (libftgl) and exits the configure script if it is not present.
#
#   Due to a bug in the Debian and Ubuntu FTGL packages, it will also
#   look for a libftgl_pic, and use it instead of libftgl if it is
#   present.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_REQUIRE_FTGL
#
# Uses pkg-config, which has a very narrow worldview, and does not
# know the difference between preprocessor and compiler, or between
# linker flags and libraries.  These are important distinctions for
# us; we'll just have to cross our fingers and hope that we never
# encounter a system where this distinction matters for FTGL.
#
# XXX should use METNO_REQUIRE_CXX_LIBRARY, if we had one.
# XXX we do, but it would create the wrong --with option.
#
AC_DEFUN([METNO_REQUIRE_FTGL], [
    METNO_REQUIRE_PKG([FTGL], [ftgl])
    saved_LIBS="${LIBS}"
    LIBS=`echo ${FTGL_LIBS} | sed -e "s/-lftgl//"`
    AC_CHECK_LIB([ftgl_pic], [main])
    AS_IF([test x$ac_cv_lib_ftgl_pic_main = xyes], [
	FTGL_LIBS="${LIBS}"
    ])
    LIBS="${saved_LIBS}"
])
