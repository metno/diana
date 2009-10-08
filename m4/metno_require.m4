# -*- autoconf -*-
#
# $Id: metno_require.m4 2788 2009-09-23 18:05:50Z dages $
#
# SYNOPSIS
#
#   XXX documentation is out of date
#
#   METNO_REQUIRE_C_HEADERS(headers)
#   METNO_REQUIRE_C_LIBRARY(library, function, header)
#
# DESCRIPTION
#
#   The METNO_REQUIRE_C_HEADERS macro verifies that the given headers
#   are compilable with a C compiler.
#
#   The METNO_REQUIRE_C_LIBRARY macro searches for a C library, and
#   exits configure if it isn't found.  It also defines --with
#   arguments, allowing the user to specify the location manually.
#
#   The macro checks for the library's presence by attempting to
#   compile a short program that calls the specified function.
#
# TODO
#
#   Need a slight rewrite.  LIB_UC should be an explicit argument;
#   LIB_LC should go away; should use AC_ARG_VAR instead of AC_SUBST.
#
#   Consider separating out the logic that checks for the library's or
#   header's presence and usability.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_REQUIRE_C_HEADER
#
# $1 = header
#
AC_DEFUN([METNO_REQUIRE_C_HEADERS], [
    # save state
    AC_LANG_PUSH(C)
    # look for each header
    m4_foreach_w(HEADER, $1, [
	AC_CHECK_HEADER(HEADER,
	    [AC_SUBST(HAVE_[]m4_translit(HEADER, [a-z/.-], [A-Z___]), [true])],
	    [AC_MSG_ERROR([the required HEADER header was not found])])
    ])
    # restore state
    AC_LANG_POP(C)
])

#
# METNO_REQUIRE_C_LIBRARY
#
# $1 = variable prefix
# $2 = library
# $3 = header
# $4 = function
#
AC_DEFUN([METNO_REQUIRE_C_LIBRARY], [
    # --with-foo magic
    METNO_WITH_LIBRARY([$1], [$2], [$2 library])

    # save state
    AC_LANG_PUSH(C)
    saved_CPPFLAGS="${CPPFLAGS}"
    saved_LDFLAGS="${LDFLAGS}"
    saved_LIBS="${LIBS}"

    # Look for the library
    METNO_FIND_LIBRARY([$1], [$2])
    AS_IF([test x"${$1_LIBDIR}" != x], [
	$1_LDFLAGS="-L${$1_LIBDIR}"
	LDFLAGS="${LDFLAGS} ${$1_LDFLAGS}"
    ])
    # Check that it works
    LIBS=""
    AC_CHECK_LIB([$2], m4_if([$4], [], [main], [$4]), [],
	[AC_MSG_ERROR([the required $2 library was not found])])
    $1_LIBS="${LIBS}"

    # Look for the header
    m4_if([$3], [], [], [
	METNO_FIND_HEADER([$1], [$3])
	AS_IF([test x"${$1_INCLUDEDIR}" != x], [
	    $1_CPPFLAGS="-I${$1_INCLUDEDIR}"
	    CPPFLAGS="${CPPFLAGS} ${$1_CPPFLAGS}"
	])
	# Check that it works
	AC_CHECK_HEADER([$3], [],
	    [AC_MSG_ERROR([the required $3 header was not found])])
    ])

    # export our stuff
    AC_SUBST([$1_CPPFLAGS])
    AC_SUBST([$1_LDFLAGS])
    AC_SUBST([$1_LIBS])

    # restore state
    LIBS="${saved_LIBS}"
    LDFLAGS="${saved_LDFLAGS}"
    CPPFLAGS="${saved_CPPFLAGS}"
    AC_LANG_POP(C)
])

#
# METNO_REQUIRE_PROG
#
# $1 = variable prefix
# $2 = program name
#
AC_DEFUN([METNO_REQUIRE_PROG], [
    AC_CHECK_PROG([$1], [$2], [$2], [])
    AS_IF([test x"${$1}" = x], [
        AC_MSG_ERROR([the required $2 program was not found])
    ])
])
