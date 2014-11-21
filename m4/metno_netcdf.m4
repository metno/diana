# -*- autoconf -*-
#
# $Id: metno_netcdf.m4 218 2010-11-04 18:41:40Z lisbethb $
#
# SYNOPSIS
#
#   METNO_REQUIRE_NETCDF(default)
#
# DESCRIPTION
#
#   The METNO_WITH_NETCDF macro searches for the netcdf and netcdf_c++
#   libraries.
#
#   The METNO_REQUIRE_NETCDF macro is equivalent, but terminates the
#   configure script if it can not find the NetCDF libraries.
#
# TODO
#
#   The METNO_REQUIRE_C_LIBRARY macro should be improved to the point
#   where this macro can simply use it instead of duplicating
#   significant parts of it. Try m4_foreach_w(COMPONENT, [$1])
#
# AUTHORS
#
#   Lisbeth Bergholt lisbethb@met.no
#

#
# METNO_WITH_NETCDF
#
# $1 = default
#
AC_DEFUN([METNO_WITH_NETCDF], [
    # --with-netcdf magic
    METNO_WITH_LIBRARY([NETCDF], [netcdf], [NetCDF library], [$1])

    # is NetCDF required, or did the user request it?
    AS_IF([test x"${with_netcdf}" != x"no" -o x"${require_netcdf}" = x"yes"], [
	# save state
	saved_CPPFLAGS="${CPPFLAGS}"
	saved_LDFLAGS="${LDFLAGS}"
	saved_LIBS="${LIBS}"
	LIBS=""

	# header location
	AS_IF([test x"${NETCDF_INCLUDEDIR}" != x""], [
	    NETCDF_CPPFLAGS="-I${NETCDF_INCLUDEDIR}"
	])
	CPPFLAGS="${CPPFLAGS} ${NETCDF_CPPFLAGS}"

	# library location
	AS_IF([test x"${NETCDF_LIBDIR}" != x""], [
	    NETCDF_LDFLAGS="-L${NETCDF_LIBDIR}"
	])
	LDFLAGS="${LDFLAGS} ${NETCDF_LDFLAGS}"

	# C version
	AC_LANG_PUSH(C)
	AC_CHECK_HEADER([netcdf.h], [], [
	    AC_MSG_ERROR([the required netcdf.h header was not found])
	])
	AC_CHECK_LIB([netcdf], [nccreate], [], [
	    AC_MSG_ERROR([the required netcdf library was not found])
	])
	AC_LANG_POP(C)

	# C++ version
	AC_LANG_PUSH(C++)
	AC_CHECK_HEADER([netcdf.hh], [], [
	    AC_MSG_ERROR([the required netcdf.hh header was not found])
	])
	AC_CHECK_LIB([netcdf_c++], [main], [], [
	    AC_MSG_ERROR([the required netcdf_c++ library was not found])
	])
	AC_LANG_POP(C++)

	# export our stuff
	NETCDF_LIBS="${LIBS}"
	AC_SUBST([NETCDF_LIBS])
	AC_SUBST([NETCDF_LDFLAGS])
	AC_SUBST([NETCDF_CPPFLAGS])

	# restore state
	LIBS="${saved_LIBS}"
	LDFLAGS="${saved_LDFLAGS}"
	CPPFLAGS="${saved_CPPFLAGS}"
    ])
])

#
# METNO_REQUIRE_NETCDF
#
AC_DEFUN([METNO_REQUIRE_NETCDF], [
    require_netcdf=yes
    METNO_WITH_NETCDF
])
