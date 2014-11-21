# -*- autoconf -*-
#
# $Id: metno_require.m4 185 2010-01-19 15:28:37Z dages $
#
# SYNOPSIS
#
#   XXX documentation is out of date
#
#   METNO_REQUIRE_C_HEADERS(headers)
#   METNO_REQUIRE_CXX_HEADERS(headers)
#   METNO_REQUIRE_C_LIBRARY(library, header, function, other-libraries)
#   METNO_REQUIRE_CXX_LIBRARY(library, header, function, other-libraries)
#
# DESCRIPTION
#
#   The METNO_REQUIRE_C_HEADERS macro verifies that the given headers
#   are compilable with a C compiler.
#
#   The METNO_REQUIRE_CXX_HEADERS macro performs the same task using a
#   C++ compiler.
#
#   The METNO_REQUIRE_C_LIBRARY macro searches for a C library, and
#   exits configure if it isn't found.  It also defines --with
#   arguments, allowing the user to specify the location manually.
#
#   The macro checks for the library's presence by attempting to
#   compile a short program that calls the specified function.
#
#   The METNO_REQUIRE_CXX_HEADERS macro performs the same task using a
#   C++ compiler.
#
# TODO
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
# METNO_REQUIRE_C_HEADERS
#
# $1 = headers
#
AC_DEFUN([METNO_REQUIRE_C_HEADERS], [
	AC_LANG_PUSH(C)
	_metno_require_c_cxx_headers([$1])
	AC_LANG_POP(C)
])

#
# METNO_REQUIRE_CXX_HEADERS
#
# $1 = headers
#
AC_DEFUN([METNO_REQUIRE_CXX_HEADERS], [
	AC_LANG_PUSH(C)
	_metno_require_c_cxx_headers([$1])
	AC_LANG_POP(C)
])

#
# Backend for METNO_REQUIRE_C_HEADER and METNO_REQUIRE_CXX_HEADER.
#
m4_define([_metno_require_c_cxx_headers], [
    # look for each header
    m4_foreach_w(HEADER, $1, [
	AC_CHECK_HEADER(HEADER,
	    [AC_SUBST(HAVE_[]m4_translit(HEADER, [a-z/.-], [A-Z___]), [true])],
	    [AC_MSG_ERROR([the required HEADER header was not found])])
    ])
])

#
# METNO_REQUIRE_C_LIBRARY
#
# $1 = variable prefix
# $2 = library
# $3 = header
# $4 = function
# $5 = other libraries
#
AC_DEFUN([METNO_REQUIRE_C_LIBRARY], [
	AC_LANG_PUSH(C)
	_metno_require_c_cxx_library([$1], [$2], [$3], [$4], [$5])
	AC_LANG_POP(C)
])

#
# METNO_REQUIRE_CXX_LIBRARY
#
# $1 = variable prefix
# $2 = library
# $3 = header
# $4 = function
# $5 = other libraries
#
AC_DEFUN([METNO_REQUIRE_CXX_LIBRARY], [
	AC_LANG_PUSH(C++)
	_metno_require_c_cxx_library([$1], [$2], [$3], [$4], [$5])
	AC_LANG_POP(C++)
])

#
# Backend for METNO_REQUIRE_C_LIBRARY and METNO_REQUIRE_CXX_LIBRARY.
#
m4_define([_metno_require_c_cxx_library], [
    # --with-foo magic
    METNO_WITH_LIBRARY([$1], [$2], [$2 library], [yes])

    # save state
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
	[AC_MSG_ERROR([the required $2 library was not found])], [$5])
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
])

#
# METNO_REQUIRE_PROG
#
# $1 = variable prefix
# $2 = program name
#
AC_DEFUN([METNO_REQUIRE_PROG], [
    AC_PATH_PROG([$1], [$2], [$2], [])
    AS_IF([test x"${$1}" = x], [
        AC_MSG_ERROR([the required $2 program was not found])
    ])
])
