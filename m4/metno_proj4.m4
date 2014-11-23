# -*- autoconf -*-
#
# metno_proj.m4    aleksandarb
#
# SYNOPSIS
#
#   METNO_REQUIRE_PROJ(default)
#
# DESCRIPTION
#
#   The METNO_WITH_PROJ macro searches for the proj library.
#
#   The METNO_REQUIRE_PROJ macro is equivalent, but terminates the
#   configure script if it can not find the proj libraries.
#
# AUTHORS
#
#   Aleksandar Babic aleksandarb@met.no
#

#
# METNO_WITH_PROJ
#
# $1 = default
#
AC_DEFUN([METNO_WITH_PROJ], [
    # --with-proj magic
    METNO_WITH_LIBRARY([PROJ], [proj], [PROJ library], [$1])

    # is PROJ required, or did the user request it?
    AS_IF([test x"${with_proj}" != x"no" -o x"${require_proj}" = x"yes"], [
	# save state
	saved_CPPFLAGS="${CPPFLAGS}"
	saved_LDFLAGS="${LDFLAGS}"
	saved_LIBS="${LIBS}"
	LIBS=""

	# header location
	AS_IF([test x"${PROJ_INCLUDEDIR}" != x""], [
	    PROJ_CPPFLAGS="-I${PROJ_INCLUDEDIR}"
	])
	CPPFLAGS="${CPPFLAGS} ${PROJ_CPPFLAGS}"

	# library location
	AS_IF([test x"${PROJ_LIBDIR}" != x""], [
	    PROJ_LDFLAGS="-L${PROJ_LIBDIR}"
		LIBS="-lproj"
	])
	LDFLAGS="${LDFLAGS} ${PROJ_LDFLAGS}"

	# C version
	AC_LANG_PUSH(C)
	AC_CHECK_HEADER([proj_api.h], [], [
	    AC_MSG_ERROR([the required proj_api.h header was not found])
	])
	AC_CHECK_LIB([proj], [pj_init], [], [
	    AC_MSG_ERROR([the required proj library was not found])
	])
	AC_LANG_POP(C)

	# export our stuff
	PROJ_LIBS="${LIBS}"
	AC_SUBST([PROJ_LIBS])
	AC_SUBST([PROJ_LDFLAGS])
	AC_SUBST([PROJ_CPPFLAGS])

	# restore state
	LIBS="${saved_LIBS}"
	LDFLAGS="${saved_LDFLAGS}"
	CPPFLAGS="${saved_CPPFLAGS}"
    ])
])

#
# METNO_REQUIRE_PROJ
#
AC_DEFUN([METNO_REQUIRE_PROJ], [
    require_proj=yes
    METNO_WITH_PROJ
])
