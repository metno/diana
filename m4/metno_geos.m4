# -*- autoconf -*-
#
# metno_geos.m4    aleksandarb
#
# SYNOPSIS
#
#   METNO_REQUIRE_GEOS(default)
#
# DESCRIPTION
#
#   The METNO_WITH_GEOS macro searches for the geos and geos_c
#   libraries.
#
#   The METNO_REQUIRE_GEOS macro is equivalent, but terminates the
#   configure script if it can not find the GEOS libraries.
#
# AUTHORS
#
#   Aleksandar Babic aleksandarb@met.no
#

#
# METNO_WITH_GEOS
#
# $1 = default
#
AC_DEFUN([METNO_WITH_GEOS], [
    # --with-geos magic
    METNO_WITH_LIBRARY([GEOS], [geos], [GEOS library], [$1])

    # is GEOS required, or did the user request it?
    AS_IF([test x"${with_geos}" != x"no" -o x"${require_geos}" = x"yes"], [
	# save state
	saved_CPPFLAGS="${CPPFLAGS}"
	saved_LDFLAGS="${LDFLAGS}"
	saved_LIBS="${LIBS}"
	LIBS=""

	# header location
	AS_IF([test x"${GEOS_INCLUDEDIR}" != x""], [
	    GEOS_CPPFLAGS="-I${GEOS_INCLUDEDIR}"
	])
	CPPFLAGS="${CPPFLAGS} ${GEOS_CPPFLAGS}"

	# library location
	AS_IF([test x"${GEOS_LIBDIR}" != x""], [
	    GEOS_LDFLAGS="-L${GEOS_LIBDIR}"
            LIBS="-lgeos -lgeos_c"
	])
	LDFLAGS="${LDFLAGS} ${GEOS_LDFLAGS}"

	# C version
	AC_LANG_PUSH(C)
	AC_CHECK_HEADER([geos_c.h], [], [
	    AC_MSG_ERROR([the required geos_c.h header was not found])
	])
	AC_CHECK_LIB([geos_c], [GEOSGetSRID], [], [
	    AC_MSG_ERROR([the required geos_c library was not found])
	])
	AC_LANG_POP(C)

	# C++ version
	AC_LANG_PUSH(C++)
	AC_CHECK_HEADER([geos.h], [], [
	    AC_MSG_ERROR([the required geos.h header was not found])
	])
	#AC_CHECK_LIB([geos], [_ZN4geos4geom11geosversionEv], [], [
	#    AC_MSG_ERROR([the required geos library was not found])
	#])
        AC_TRY_COMPILE([
                           #include <geos/geom/PrecisionModel.h>
                           using namespace geos::geom;
                       ],
                       [
                           PrecisionModel dummy;
                       ],
                       [], 
                       [
	                   AC_MSG_ERROR([PrecisionModel was not found])
	               ])
	AC_LANG_POP(C++)

	# export our stuff
	GEOS_LIBS="${LIBS}"
	AC_SUBST([GEOS_LIBS])
	AC_SUBST([GEOS_LDFLAGS])
	AC_SUBST([GEOS_CPPFLAGS])

	# restore state
	LIBS="${saved_LIBS}"
	LDFLAGS="${saved_LDFLAGS}"
	CPPFLAGS="${saved_CPPFLAGS}"
    ])
])

#
# METNO_REQUIRE_GEOS
#
AC_DEFUN([METNO_REQUIRE_GEOS], [
    require_geos=yes
    METNO_WITH_GEOS
])
