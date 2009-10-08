# -*- autoconf -*-
#
# $Id: metno_require.m4 2766 2009-09-14 04:44:08Z dages $
#
# SYNOPSIS
#
#   METNO_CHECK_PKG(prefix, library, static)
#   METNO_REQUIRE_PKG(prefix, library, static)
#
# DESCRIPTION
#
#   The METNO_CHECK_PKG macro is an alternative to pkg-config's
#   PKG_CHECK_MODULES macro.  It does mostly the same job, with a few
#   differences:
#
#   The METNO_REQUIRE_PKG macro is a wrapper around METNO_CHECK_PKG
#   that terminates the configure script if the specified package is
#   not found.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_PROG_PKG_CONFIG
#
AC_DEFUN([METNO_PROG_PKG_CONFIG], [
    AC_ARG_VAR([PKG_CONFIG], [location of the pkg-config utility])
    AC_ARG_VAR([PKG_CONFIG_PATH], [path(s) to pkg-config files])
    AS_IF([test x"${PKG_CONFIG}" = x], [
	AC_PATH_TOOL([PKG_CONFIG], [pkg-config])
    ])
])

#
# METNO_CHECK_PKG
#
# $1 = variable-prefix
# $2 = library-name
# $3 = minimum-version
# $4 = static
#
AC_DEFUN([METNO_CHECK_PKG], [
    AC_MSG_CHECKING([m4_if([$3], [], [for $2], [for $2 >= $3])])
    metno_$2_pkg_atleast="m4_if([$3], [], [], [--atleast-version $3])"
    metno_$2_pkg_static="m4_if([$4], [static], [--static], [])"
    metno_$2_pkg_cmd="${PKG_CONFIG} ${metno_$2_pkg_atleast} ${metno_$2_pkg_static}"
    AS_IF([${metno_$2_pkg_cmd} --exists $2], [
	AC_SUBST([$1_VERSION], [`${metno_$2_pkg_cmd} --modversion $2`])
	AC_MSG_RESULT([${$1_VERSION}])
	AC_SUBST([$1_CPPFLAGS], [`${metno_$2_pkg_cmd} --cflags $2`])
	AC_SUBST([$1_LDFLAGS], [`${metno_$2_pkg_cmd} --libs-only-L $2`])
	AC_SUBST([$1_LIBS], [`${metno_$2_pkg_cmd} --libs-only-l $2`])
	# XXX what about only-other?
    ], [
	AC_MSG_RESULT([no])
    ])
])

#
# METNO_REQUIRE_PKG
#
# $1 = variable-prefix
# $2 = library-name
# $3 = minimum-version
# $4 = static
#
AC_DEFUN([METNO_REQUIRE_PKG], [
    METNO_CHECK_PKG([$1], [$2], [$3], [$4])
    AS_IF([test x"${$1_VERSION}" = x], [
	AC_MSG_ERROR([the required $2 library was not found])
    ])
])
