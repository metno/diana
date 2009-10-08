# -*- autoconf -*-
#
# $Id: metno_hdf.m4 2766 2009-09-14 04:44:08Z dages $
#
# SYNOPSIS
#
#   METNO_REQUIRE_HDF4
#   METNO_REQUIRE_HDF5
#
# DESCRIPTION
#
#   The METNO_REQUIRE_HDF4 macro searches for the HDF4 libraries
#   (libdf and libmfhdf) and terminates the configure script if they
#   are not present.
#
#   The METNO_REQUIRE_HDF5 macro searches for the HDF5 libraries
#   (libhdf5, libhdf5_cpp and libhdf5_hl) and terminates the configure
#   script if they are not present.
#
# TODO
#
#   The METNO_REQUIRE_C_LIBRARY macro should be improved to the point
#   where these macros can simply use it instead of duplicating
#   significant parts of it.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_REQUIRE_HDF4
#
# HDF4 consists of two libraries.  We will assume that they are both
# in the same location, and that their headers are all in the same
# location, so we generate only one set of command-line options,
# --with-hdf4*, instead of a separate set for each library.  Note that
# HDF4 calls itself HDF, not HDF4, and we need to take that into
# account when searching for headers and libraries.
#
AC_DEFUN([METNO_REQUIRE_HDF4], [
    # --with-hdf4 magic
    METNO_WITH_LIBRARY([HDF4], [hdf4], [HDF 4 library])
    # save state
    AC_LANG_PUSH(C)
    saved_CPPFLAGS="${CPPFLAGS}"
    saved_LDFLAGS="${LDFLAGS}"
    saved_LIBS="${LIBS}"
    # check for stuff from --with-hdf4
    AS_IF([test x"${HDF4_INCLUDEDIR}" != x], [
	CPPFLAGS="${CPPFLAGS} -I${HDF4_INCLUDEDIR}"
    ])
    AS_IF([test x"${HDF4_LIBDIR}" != x], [
	LDFLAGS="${LDFLAGS} -L${HDF4_LIBDIR}"
	LIBS="-ldf -lmfhdf"
    ])
    # look for the header file
    METNO_FIND_HEADER([HDF4], [hdf.h], [hdf])
    AS_IF([test x"${HDF4_INCLUDEDIR}" != x], [
	CPPFLAGS="${CPPFLAGS} -I${HDF4_INCLUDEDIR}"
    ], [
	AC_MSG_ERROR([unable to locate HDF4 include directory])
    ])
    HDF4_CPPFLAGS="${CPPFLAGS}"
    # find our libraries
    # XXX use METNO_FIND instead
    LIBS=""
    AC_CHECK_LIB([df], [Hopen], [],
	[AC_MSG_ERROR([the required df library (part of hdf4) was not found])])
    AC_CHECK_LIB([mfhdf], [SDstart], [],
	[AC_MSG_ERROR([the required mfhdf library (part of hdf4) was not found])])
    HDF4_LDFLAGS="${LDFLAGS}"
    HDF4_LIBS="${LIBS}"
    # export our stuff
    AC_SUBST([HDF4_CPPFLAGS])
    AC_SUBST([HDF4_LDFLAGS])
    AC_SUBST([HDF4_LIBS])
    # restore state
    LIBS="${saved_LIBS}"
    LDFLAGS="${saved_LDFLAGS}"
    CPPFLAGS="${saved_CPPFLAGS}"
    AC_LANG_POP(C)
])

#
# METNO_REQUIRE_HDF5
#
# HDF5 consists of several libraries.  We will concentrate on those
# used by Metlibs and Diana.  We will assume that they are all in the
# same location, and that their headers are all in the same location,
# so we generate only one set of command-line options, --with-hdf5*,
# instead of a separate set for each library.
#
AC_DEFUN([METNO_REQUIRE_HDF5], [
    # --with-hdf5 magic
    METNO_WITH_LIBRARY([HDF5], [hdf5], [HDF 5 library])
    # save state
    AC_LANG_PUSH(C++)
    saved_CPPFLAGS="${CPPFLAGS}"
    saved_LDFLAGS="${LDFLAGS}"
    saved_LIBS="${LIBS}"
    # check for stuff from --with-hdf5
    AS_IF([test x"${HDF5_INCLUDEDIR}" != x], [
	CPPFLAGS="${CPPFLAGS} -I${HDF5_INCLUDEDIR}"
    ])
    AS_IF([test x"${HDF5_LIBDIR}" != x], [
	LDFLAGS="${LDFLAGS} -L${HDF5_LIBDIR}"
	LIBS="-ldf -lmfhdf"
    ])
    # look for the header file
    METNO_FIND_HEADER([HDF5], [hdf5.h], [hdf5])
    AS_IF([test x"${HDF5_INCLUDEDIR}" != x], [
	CPPFLAGS="${CPPFLAGS} -I${HDF5_INCLUDEDIR}"
    ], [
	AC_MSG_ERROR([unable to locate HDF5 include directory])
    ])
    HDF5_CPPFLAGS="${CPPFLAGS}"
    # find our libraries
    # XXX need a good way to check for a C++ library
    # XXX use METNO_FIND instead
    LIBS=""
    AC_CHECK_LIB([hdf5], [H5check_version], [],
	[AC_MSG_ERROR([the required hdf5 library (part of hdf5) was not found])])
    AC_CHECK_LIB([hdf5_hl], [H5TBmake_table], [],
	[AC_MSG_ERROR([the required hdf5_hl library (part of hdf5) was not found])])
    AC_CHECK_LIB([hdf5_cpp], [main], [],
	[AC_MSG_ERROR([the required hdf5_cpp library (part of hdf5) was not found])])
    HDF5_LDFLAGS="${LDFLAGS}"
    HDF5_LIBS="${LIBS}"
    # export our stuff
    AC_SUBST([HDF5_CPPFLAGS])
    AC_SUBST([HDF5_LDFLAGS])
    AC_SUBST([HDF5_LIBS])
    # restore state
    LIBS="${saved_LIBS}"
    LDFLAGS="${saved_LDFLAGS}"
    CPPFLAGS="${saved_CPPFLAGS}"
    AC_LANG_POP(C++)
])
