
# Usage:
# GTEST_CHECK([compile_location])
# If gtest headers are found, but not libraries, googletest may be 
# automatically compiled in the given location - but you need to provide 
# makefiles/rules for that yourself. The path will be relative to top_builddir.
#
# Example:
# configure.ac:
#  AC_PATH_PROG(CMAKE, cmake, false)
#  GTEST_CHECK([test/gtest])
#
# test/gtest/Makefile.am
#  if MUST_COMPILE_GTEST
#  
#  gtestdir = 
#  gtest_LIBRARIES = libgtest.a libgtest_main.a
#  
#  libgtest_a_SOURCES = 
#  libgtest_main_a_SOURCES = 
#  
#  $(gtest_LIBRARIES): compile_gtest
#  	cp gtest/$@ .
#  
#  if MUST_COMPILE_GTEST
#  libgtest.a: ${gtest_src}/src/gtest-all.cc
#  	$(CXX) $(gtest_CPPFLAGS) -I$(gtest_src) -pthread -c $<
#  	$(AR) cru $@ gtest-all.o
#  
#  libgtest_main.a: ${gtest_src}/src/gtest_main.cc
#  	$(CXX) $(gtest_CPPFLAGS) -pthread -c $<
#  	$(AR) cru $@ gtest_main.o
#  endif # MUST_COMPILE_GTEST

AC_DEFUN([GTEST_CHECK],
[
AC_ARG_WITH([gtest],
    [AS_HELP_STRING([--with-gtest], [Specify google test directory])],
    [gtest_base=${with_gtest}; gtest_src="${gtest_base}"],
    [gtest_base=/usr;          gtest_src="${gtest_base}/src/gtest"])

cppflags_old="${CPPFLAGS}"
ldflags_old="${LDFLAGS}"
libs_old="${LIBS}"

header_gtest="gtest/gtest.h"
libs_gtest="-lgtest -lpthread"
gtest_compile_dir="$1"

AC_LANG_PUSH(C++)
# check if gtest header is found, set have_gtest to true/false
AS_IF([test "x$gtest_base" = "x/usr"],
    [],
    [gtest_cppflags="-I${gtest_base}/include"])
CPPFLAGS="${CPPFLAGS} ${gtest_cppflags}"
AC_CHECK_HEADER([${header_gtest}],
    [gtest_CPPFLAGS="${gtest_cppflags}"
    have_gtest=true],
    [AC_MSG_WARN([Unable to find googletest header '${header_gtest}'])
    have_gtest=false])

if test "x$have_gtest" = "xtrue"; then # header was found

# check linking to see if there is a precompiled library
AS_IF([test "x$gtest_base" = "x/usr"],
    [],
    [gtest_ldflags="-L${gtest_base}/lib"])
LDFLAGS="${LDFLAGS} ${gtest_ldflags}"
LIBS="${LIBS} ${libs_gtest}"
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <${header_gtest}>], [])],
	[gtest_LIBS="${libs_gtest}"
	 gtest_LDFLAGS="${gtest_ldflags}"],
	[
	if test "${gtest_compile_dir}"; then
		AC_MSG_NOTICE([Found headers but no precompiled googletest libraries - must compile own version in '${gtest_compile_dir}'])
		must_compile_gtest=true
		gtest_LIBS="${libs_gtest}"
		gtest_LDFLAGS="-L\$(top_builddir)/${gtest_compile_dir}"
	else
		AC_MSG_WARN([Found headers but no precompiled googletest libraries - unable to use googletest])
		have_gtest=false
	fi
])

fi # have_gtest was true because header was found
AC_LANG_POP

CPPFLAGS="${cppflags_old}"
LDFLAGS="${ldflags_old}"
LIBS="${libs_old}"

AM_CONDITIONAL(HAVE_GTEST, [test x${have_gtest} = xtrue])
AM_CONDITIONAL(MUST_COMPILE_GTEST, [test x${must_compile_gtest} = xtrue])

AC_SUBST(gtest_CPPFLAGS)
AC_SUBST(gtest_LDFLAGS)
AC_SUBST(gtest_LIBS)
AC_SUBST(gtest_src)
])

########################################################################

# Usage:
# GMOCK_DIST_CHECK([compile_location])
# If gmock headers are found, but not libraries, gmock may be 
# automatically compiled in the given location - but you need to provide 
# makefiles/rules for that yourself. The path will be relative to top_builddir.

AC_DEFUN([GMOCK_CHECK],
[
AS_IF([test "x$have_gtest" = "x"],
    [GTEST_CHECK($1)])

AC_ARG_WITH([gmock],
    [AS_HELP_STRING([--with-gmock], [Specify google gmock directory (might be used for gtest too)])],
    [gmock_base=${with_gmock}; gmock_src="${gmock_base}"],
    [gmock_base=/usr;          gmock_src="${gmock_base}/src/gmock"])

cppflags_old="${CPPFLAGS}"
ldflags_old="${LDFLAGS}"
libs_old="${LIBS}"

# gmock not useful without gtest
AS_IF([test "x$have_gtest" = "xtrue"],[

header_gmock="gmock/gmock.h"
libs_gmock="-lgmock"
gmock_compile_dir="$1"

AC_LANG_PUSH(C++)
# check if gmock header is found, set have_gmock to true/false
AS_IF([test "x$gmock_base" = "x/usr"],
    [],
    [gmock_cppflags="-I${gmock_base}/include"])
CPPFLAGS="${CPPFLAGS} ${gmock_cppflags} ${gtest_CPPFLAGS}"
AC_CHECK_HEADER([${header_gmock}],
    [gmock_CPPFLAGS="${gmock_cppflags}"
    have_gmock=true],
    [AC_MSG_WARN([Unable to find gmock header '${header_gmock}'])
    have_gmock=false])

if test "x$have_gmock" = "xtrue"; then # header was found

# check linking to see if there is a precompiled library
AS_IF([test "x$gmock_base" = "x/usr"],
    [],
    [gmock_ldflags="-L${gmock_base}/lib"])
AS_IF([test "x$must_compile_gtest" = "xtrue"],
    [# gtest not compiled yet, cannot link
     LDFLAGS="${LDFLAGS} ${gmock_ldflags}"
     LIBS="${LIBS} ${libs_gmock}"],
    [# gtest library available, link to it
     LDFLAGS="${LDFLAGS} ${gmock_ldflags} ${gtest_LDFLAGS}"
     LIBS="${LIBS} ${libs_gmock} ${gtest_LIBS}"])
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <${header_gmock}>], [])],
	[gmock_LIBS="${libs_gmock}"
	 gmock_LDFLAGS="${gmock_ldflags}"],
	[
	if test "${gmock_compile_dir}"; then
		AC_MSG_NOTICE([Found headers but no precompiled gmock libraries - must compile own version in '${gmock_compile_dir}'])
		must_compile_gmock=true
		gmock_LIBS="${libs_gmock}"
		gmock_LDFLAGS="-L\$(top_builddir)/${gmock_compile_dir}"
	else
		AC_MSG_WARN([Found headers but no precompiled gmock libraries - unable to use gmock])
		have_gmock=false
	fi
])
gmock_CPPFLAGS="${gmock_CPPFLAGS} ${gtest_CPPFLAGS}"
gmock_LIBS="${gmock_LIBS} ${gtest_LIBS}"
gmock_LDFLAGS="${gmock_LDFLAGS} ${gtest_LDFLAGS}"

fi # have_gmock was true because header was found
AC_LANG_POP

]) # have_gtest, ie. gtest was found

CPPFLAGS="${cppflags_old}"
LDFLAGS="${ldflags_old}"
LIBS=${libs_old}

AM_CONDITIONAL(HAVE_GMOCK, [test x${have_gmock} = xtrue])
AM_CONDITIONAL(MUST_COMPILE_GMOCK, [test x${must_compile_gmock} = xtrue])

AC_SUBST(gmock_CPPFLAGS)
AC_SUBST(gmock_LDFLAGS)
AC_SUBST(gmock_LIBS)
AC_SUBST(gmock_src)
])
