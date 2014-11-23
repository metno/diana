# -*- autoconf -*-
#
# $Id: metno_pkg.m4 243 2014-04-08 11:53:58Z alexanderb $
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
AC_DEFUN_ONCE([METNO_PROG_PKG_CONFIG], [
    AC_ARG_VAR([PKG_CONFIG], [location of the pkg-config utility])
    AC_ARG_VAR([PKG_CONFIG_PATH], [path(s) to pkg-config files])
    AS_IF([test x"${PKG_CONFIG}" = x], [
	AC_PATH_TOOL([PKG_CONFIG], [pkg-config])
    ])
    AS_IF([test x"${PKG_CONFIG}" = x], [
	AC_MSG_ERROR([unable to locate pkg-config])
    ])
    AC_MSG_CHECKING([for pkg-config path])
    AS_IF([test x"${PKG_CONFIG_PATH}" = x], [
	PKG_CONFIG_PATH=${prefix}/lib/pkgconfig
    ])
    export PKG_CONFIG_PATH
    AC_MSG_RESULT([$PKG_CONFIG_PATH])
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
    AC_REQUIRE([METNO_PROG_PKG_CONFIG])
    AC_MSG_CHECKING([m4_if([$3], [], [for $2], [for $2 >= $3])])
    metno_$2_pkg_version="m4_if([$3], [], [--exists], [--atleast-version $3])"
    metno_$2_pkg_static="m4_if([$4], [static], [--static], [])"
    metno_$2_pkg_cmd="${PKG_CONFIG} ${metno_$2_pkg_static}"
    AS_IF([${metno_$2_pkg_cmd} ${metno_$2_pkg_version} $2], [
	AC_SUBST([$1_VERSION], [`${metno_$2_pkg_cmd} --modversion $2`])
	AC_MSG_RESULT([${$1_VERSION}])
	AC_SUBST([$1_CPPFLAGS], [`${metno_$2_pkg_cmd} --cflags $2`])
	AC_SUBST([$1_LDFLAGS], [`${metno_$2_pkg_cmd} --libs-only-L $2`])
	AC_SUBST([$1_LIBS], [`${metno_$2_pkg_cmd} --libs-only-l $2`])
        if test -n "$METNO_PKG_REQUIRED"; then
            METNO_PKG_REQUIRED="${METNO_PKG_REQUIRED}, "
        fi
        METNO_PKG_REQUIRED="${METNO_PKG_REQUIRED}$2"
        if test -n "$3"; then
            METNO_PKG_REQUIRED="${METNO_PKG_REQUIRED} >= $3"
        fi
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
    AC_REQUIRE([METNO_PROG_PKG_CONFIG])
    METNO_CHECK_PKG([$1], [$2], [$3], [$4])
    AS_IF([test x"${$1_VERSION}" = x], [
	AC_MSG_ERROR([the required $2 library was not found])
    ])
])
