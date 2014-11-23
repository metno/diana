# -*- autoconf -*-
#
# $Id: metno_search.m4 190 2010-01-20 12:00:00Z dages $
#
# SYNOPSIS
#
#   METNO_SEARCH_LIBRARIES(variable-prefix, libraries,
#       headers, code, other-libraries, if-yes, if-no)
#
# DESCRIPTION
#
#   The METNO_SEARCH_LIBRARIES macro is similar to AC_SEARCH_LIBS,
#   with a few important differences:
#
#   - While AC_SEARCH_LIBS is intended to be used to determine which
#     library provides a particular function (e.g. socket() is in -lc
#     on BSD and GNU systems but in -lsocket on Solaris), whereas
#     METNO_SEARCH_LIBRARIES is intended to be used to locate a
#     particular library which may be present under one of several
#     different names (e.g. -lGL vs. -lopengl32).
#
#   - METNO_SEARCH_LIBRARIES does not try to link the test program
#     without any libraries at all.
#
#   - METNO_SEARCH_LIBRARIES allows the caller to specify a list of
#     headers to include in the test program.
#
#   - METNO_SEARCH_LIBRARIES allows the caller to specify arbitrary
#     code to include in the program, not just a function name.
#
#   The last two items mean that it is possible to search for C++
#   libraries that don't export any pure C symbols by instantiating a
#   class or a template.  It also improves support for Windows DLLs,
#   where some symbols may be visible only if a complete declaration
#   is in scope, due to calling convention issues.
#
#   If the test program links against one of the provided libraries,
#   the <variable-prefix>_LIBS variable is set accordingly and
#   AC_SUBSTed.
#
#   If <variable-prefix>_CPPFLAGS and / or <variable-prefix>_LDFLAGS
#   are set (e.g. by a previous call to METNO_WITH_LIBRARY), they are
#   used when compiling and linking the test program.
#
#   Note that a bare function or variable name is a valid (though
#   ineffectual) statement in both C and C++, provided that a proper
#   declaration of that function or variable is in scope.
#
#   This macro can be called multiple times for multiple libraries;
#   the appropriate flags will accumulate in <variable-prefix>_LIBS.
#   This is useful when searching for multiple libraries that
#   constitute a single logical unit.
#
# TODO
#
#   Improve documentation.
#
#   Consider merging with METNO_REQUIRE_*_LIBRARY.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_SEARCH_LIBRARIES
#
# $1 = variable-prefix
# $2 = libraries
# $3 = headers
# $4 = code
# $5 = other-libraries
# $6 = if-yes
# $7 = if-no
#
AC_DEFUN([METNO_SEARCH_LIBRARIES], [
    # save state
    metno_search_libraries_CPPFLAGS="${CPPFLAGS}"
    metno_search_libraries_LDFLAGS="${LDFLAGS}"
    metno_search_libraries_LIBS="${LIBS}"
    CPPFLAGS="${CPPFLAGS} ${$1_CPPFLAGS}"
    LDFLAGS="${LDFLAGS} ${$1_LDFLAGS}"
    $1_LIBS=""
    # look for each library in turn
    for lib in $2 ; do
	AC_MSG_CHECKING([for $lib library])
	LIBS="-l$lib $5"
	AC_LINK_IFELSE([
	    AC_LANG_PROGRAM([
		m4_foreach_w([header], $3, [
@%:@include <header>])
	    ], [
		$4;
	    ])
	], [
	    AC_MSG_RESULT([yes])
	    $1_LIBS="-l$lib ${$1_LIBS}"
	    # stop at first hit
	    break
	], [
	    AC_MSG_RESULT([no])
	])
	LIBS=""
    done
    # export our stuff
    AC_SUBST([$1_LIBS])
    # restore state
    LIBS="${metno_search_libraries_LIBS}"
    LDFLAGS="${metno_search_libraries_LDFLAGS}"
    CPPFLAGS="${metno_search_libraries_CPPFLAGS}"
    AS_IF([test x"${$1_LIBS}" != x""], [:;$6], [:;$7])
])
