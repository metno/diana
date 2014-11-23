# -*- autoconf -*-
#
# $Id: metno_boost.m4 244 2014-04-25 11:29:55Z juergens $
#
# SYNOPSIS
#
#   METNO_REQUIRE_BOOST(features)
#
# DESCRIPTION
#
#   The Boost libraries are an heterogenous amalgamation of class and
#   template libraries of highly uneven quality.  Some components
#   consist exclusively of header files, while others consist of a C++
#   library accompanied by header files.  The entire mess is
#   distributed as a single source tarball, but some OS distributions
#   split it up into separate packages along more or less arbitrary
#   binaries.  We assume that all Boost components are installed in
#   the same location.
#
#   The METNO_REQUIRE_BOOST and METNO_REQUIRE_BOOST_MT macros create a
#   --with-boost configuration option and set BOOST_CPPFLAGS and
#   BOOST_LDFLAGS accordingly.  The MT variant causes subsequent
#   METNO_REQUIRE_BOOST_FEATURE tests to look for multi-threaded
#   versions of the Boost libraries.
#
#   The METNO_REQUIRE_BOOST_FEATURE macro verifies that a specific
#   component (e.g. boost_date_time, boost_serialization) is present,
#   and sets BOOST_*_LIBS accordingly.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_REQUIRE_BOOST
#
AC_DEFUN_ONCE([METNO_REQUIRE_BOOST], [
    # --with-boost magic
    METNO_WITH_LIBRARY([BOOST], [boost], [Boost], [yes])
    AS_IF([test x"${BOOST_INCLUDEDIR}" != x""],
	[CPPFLAGS="${CPPFLAGS} -I${BOOST_INCLUDEDIR}"])
    AS_IF([test x"${BOOST_LIBDIR}" != x""],
	[LDFLAGS="${LDFLAGS} -L${BOOST_LIBDIR}"])
    AC_SUBST([BOOST_LDFLAGS])
    AC_SUBST([BOOST_CPPFLAGS])
])

#
# METNO_REQUIRE_BOOST_MT
#
AC_DEFUN([METNO_REQUIRE_BOOST_MT], [
    AC_REQUIRE([METNO_REQUIRE_BOOST])
    AC_CHECK_LIB(boost_thread-mt, main, [METNO_BOOST_MT_SUFFIX="-mt" AC_SUBST(METNO_BOOST_MT_SUFFIX)])
])

#
# METNO_REQUIRE_BOOST_FEATURE
#
# $1 = feature
# $2 = header name if not boost/$1.hpp
#
AC_DEFUN([METNO_REQUIRE_BOOST_FEATURE], [
    AC_REQUIRE([METNO_REQUIRE_BOOST])
    # save state
    AC_LANG_PUSH(C++)
    AS_VAR_PUSHDEF([FEATURE_LIBS], [m4_translit([BOOST_$1_LIBS], [a-z], [A-Z])])
    saved_CPPFLAGS="${CPPFLAGS}"
    saved_LDFLAGS="${LDFLAGS}"
    saved_LIBS="${LIBS}"
    LIBS=""
    m4_bmatch([$1],
    [date_time\|regex\|thread\|program_options\|filesystem\|serialization\|system], [
	AC_CHECK_LIB([boost_$1${METNO_BOOST_MT_SUFFIX}], [main],
	    [FEATURE_LIBS=-lboost_$1${METNO_BOOST_MT_SUFFIX}],
	    [AC_MSG_ERROR([the required Boost component $1 was not found])])
    ],
    [smart_ptr], [
	# header only
    ],
    # should be an autoconf error not a configure error
    # should the default case simply be "header only"?
    [AC_MSG_ERROR([unknown Boost component: $1])])
    AC_CHECK_HEADER(m4_if([$2], [], [boost/$1.hpp], $2), [],
	[AC_MSG_ERROR([the required Boost component $1 was not found])])
    AC_SUBST(FEATURE_LIBS)
    # restore state
    LIBS="${saved_LIBS}"
    LDFLAGS="${saved_LDFLAGS}"
    CPPFLAGS="${saved_CPPFLAGS}"
    AS_VAR_POPDEF([FEATURE_LIBS])
    AC_LANG_POP(C++)
])
