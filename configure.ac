#
# $Id$
#
AC_PREREQ([2.59])
AC_INIT([diana], [3.18.0], [diana@met.no])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

METNO_SET_VERSION_INFO
METNO_PROGRAM_SUFFIX

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
#AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET
METNO_PROG_PKG_CONFIG

#
# C
#
AC_LANG_PUSH([C])
AC_LANG_POP([C])

#
# C++
#
AC_LANG_PUSH([C++])

AC_CXX_HAVE_STD
AC_CXX_HAVE_STL

AC_LANG_POP([C++])

#
# Features
#

AC_ARG_ENABLE([roadobs],
    [AS_HELP_STRING([--enable-roadobs], [enable ROAD observations])],
    [], [enable_roadobs=no])
AM_CONDITIONAL([WITH_ROADOBS], [test x$enable_roadobs = xyes])

AC_ARG_ENABLE([gribfile],
    [AS_HELP_STRING([--enable-gribfile], [enable Gribfile support])],
    [], [enable_gribfile=no])
AM_CONDITIONAL([WITH_GRIBAPI], [test x$enable_gribfile = xyes])

AC_ARG_ENABLE([geotiff],
    [AS_HELP_STRING([--enable-geotiff], [enable GeoTiff support])],
    [], [enable_geotiff=no])
AM_CONDITIONAL([WITH_GEOTIFF], [test x$enable_geotiff = xyes])

AC_ARG_ENABLE([profet],
    [AS_HELP_STRING([--enable-profet], [enable Profet])],
    [], [enable_profet=no])
AM_CONDITIONAL([WITH_PROFET], [test x$enable_profet = xyes])

AC_ARG_ENABLE([netcdf],
    [AS_HELP_STRING([--disable-netcdf], [disable NetCDF support])],
    [], [enable_netcdf=yes])
AM_CONDITIONAL([WITH_NETCDF], [test x$enable_netcdf = xyes])

AC_ARG_ENABLE([hdf5],
    [AS_HELP_STRING([--enable-hdf5], [enable HDF5 support])],
    [], [enable_hdf5=no])
AM_CONDITIONAL([WITH_HDF5], [test x$enable_hdf5 = xyes])

AC_ARG_ENABLE([obs-bufr],
    [AS_HELP_STRING([--disable-obs-bufr], [disable observation buffers])],
    [], [enable_obs_bufr=no])
AM_CONDITIONAL([WITH_OBS_BUFR], [test x$enable_obs_bufr = xyes])

AC_ARG_ENABLE([video-export],
    [AS_HELP_STRING([--disable-video-export], [disable video export feature])],
    [], [enable_video_export=yes])
AM_CONDITIONAL([WITH_VIDEO_EXPORT], [test x$enable_video_export = xyes])

AC_ARG_ENABLE([xlib],
    [AS_HELP_STRING([--enable-xlib], [enable X-lib usage])],
    [], [enable_xlib=no])
AM_CONDITIONAL([WITH_XLIB], [test x$enable_xlib = xyes])

#
# metlibs
#
# there must be a more elegant way to do this
METNO_REQUIRE_PKG([DIFIELD], [diField])
METNO_REQUIRE_PKG([DIMITIFF], [diMItiff])
METNO_REQUIRE_PKG([DISQL], [diSQL])
METNO_REQUIRE_PKG([DIWDB], [diWdb])
METNO_REQUIRE_PKG([GLTEXT], [glText])
METNO_REQUIRE_PKG([GLP], [glp])
METNO_REQUIRE_PKG([LIBMI], [libmi])
METNO_REQUIRE_PKG([MILOGGER], [miLogger])
METNO_REQUIRE_PKG([MILIB], [milib])
METNO_REQUIRE_PKG([PARAMETER], [parameter])
METNO_REQUIRE_PKG([PROPOLY],[propoly])
METNO_REQUIRE_PKG([PUCTOOLS], [puCtools])
METNO_REQUIRE_PKG([PUDATATYPES], [puDatatypes])
METNO_REQUIRE_PKG([PUSQL], [puSQL])
METNO_REQUIRE_PKG([PUTOOLS], [puTools])
METNO_REQUIRE_PKG([QUTILITIES], [qUtilities])
METNO_REQUIRE_PKG([ROBS], [robs])

AS_IF([test x$enable_gribfile = xyes], [
    METNO_REQUIRE_PKG([GRIBAPI], [GribAPI])
])

AS_IF([test x$enable_roadobs = xyes], [
    METNO_REQUIRE_PKG([RDKAPI], [rdkAPI])
    METNO_REQUIRE_PKG([ROADAPI], [roadAPI])
])

AS_IF([test x$enable_profet = xyes], [
    METNO_REQUIRE_PKG([PODS], [pods])
    METNO_REQUIRE_PKG([PROFET], [profet])
    METNO_REQUIRE_PKG([PROFUNCTIONS], [proFunctions])
])

#
# Other third-party dependencies
#
METNO_REQUIRE_QT4([
    Core
    Gui
    Network
    OpenGL
])

METNO_REQUIRE_FTGL

#mySQL
AX_LIB_MYSQL()
AC_SUBST([MYSQL_CPPFLAGS], [${MYSQL_CFLAGS}])
MYSQL_CFLAGS=
AC_SUBST([MYSQL_LIBS], [${MYSQL_LDFLAGS}])
MYSQL_LDFLAGS=

# libavformat for video production
AS_IF([test x$enable_video_export = xyes], [
    METNO_REQUIRE_C_LIBRARY([AVFORMAT], [avformat], [ffmpeg/avformat.h], [av_read_frame])
])


# shapefile
METNO_REQUIRE_C_LIBRARY([SHP], [shp], [], [SHPOpen])

# EMOSLIB
AS_IF([test x$enable_obs_bufr = xyes], [
    METNO_REQUIRE_C_LIBRARY([EMOS], [emos], [], [pbopen])
])

#
# Output
#

# creating bin/diana from bin/diana.in - substituting test stuff
AC_SUBST([program_suffix])
AC_CONFIG_FILES([bin/diana])


AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_OUTPUT
