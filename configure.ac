#
# $Id: configure.ac 4440 2014-09-04 12:05:03Z lisbethb $
#
AC_PREREQ([2.59])
AC_INIT([diana], [3.37.0~rc9], [diana@met.no])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.10 foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

METNO_PVERSION
METNO_PROGRAM_SUFFIX

AC_LANG([C])
AC_LANG([C++])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET
METNO_PROG_PKG_CONFIG

#
# Win32 support
#
METNO_WIN32([0x0501])
METNO_WIN32_NO_GDI
METNO_WIN32_NO_UNICODE
METNO_WIN32_WINSOCK

#
# C
#
AC_LANG_PUSH([C])

METNO_WIN32_IFELSE([DEBUG_FLAGS=-gstabs], [DEBUG_FLAGS=-g])
CFLAGS="$CFLAGS -O2 -pipe $DEBUG_FLAGS -Wall -Wextra -fPIC -fopenmp "
METNO_WIN32_IFELSE([OPT_FLAGS="-msse2 -march=pentium4"], [OPT_FLAGS="-msse2 -march=core2"])
CXXFLAGS="$CXXFLAGS ${OPT_FLAGS} -Wall -fPIC -fopenmp "
AC_SYS_LARGEFILE

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
AC_ARG_ENABLE([smhi],
    [AS_HELP_STRING([--enable-smhi], [enable SMHI symbols])],
    [], [enable_smhi=no])
AM_CONDITIONAL([WITH_SMHI], [test x$enable_smhi = xyes])

AC_ARG_ENABLE([newarkobs],
    [AS_HELP_STRING([--enable-newarkobs], [enable NEWARK observations])],
    [], [enable_newarkobs=no])
AM_CONDITIONAL([WITH_NEWARKOBS], [test x$enable_newarkobs = xyes])

AC_ARG_ENABLE([geotiff],
    [AS_HELP_STRING([--enable-geotiff], [enable GeoTiff support])],
    [], [enable_geotiff=no])
AM_CONDITIONAL([WITH_GEOTIFF], [test x$enable_geotiff = xyes])

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

AC_ARG_ENABLE([embedded],
    [AS_HELP_STRING([--enable-embedded], [enable embedded usage (removes X and OpenGL dependencies)])],
	[], [enable_embedded=no])
AM_CONDITIONAL([WITH_EMBEDDED], [test x$enable_embedded = xyes])

AC_ARG_ENABLE([batch-only],
    [AS_HELP_STRING([--enable-batch-only], [only compile the batch version of diana (bdiana)])],
        [], [enable_batch_only=no])
AM_CONDITIONAL([WITH_BATCH_ONLY], [test x$enable_batch_only = xyes])

AC_ARG_ENABLE([perl],
    [AS_HELP_STRING([--enable-perl], [enable perl module)])],
	    [], [enable_perl=no])
AM_CONDITIONAL([WITH_PERL], [test x$enable_perl = xyes])


#
# metlibs
#
# there must be a more elegant way to do this
METNO_REQUIRE_PKG([DIFIELD], [diField])
METNO_REQUIRE_PKG([MIRASTER], [miRaster])

AS_IF([test x$enable_embedded != xyes], [
    METNO_REQUIRE_PKG([GLTEXT], [glText])
    METNO_REQUIRE_PKG([GLP], [glp])
    METNO_REQUIRE_PKG([GLU],[glu])
])

AS_IF([test x$enable_embedded != xyes], [
    METNO_REQUIRE_PKG([MIFTGL], [miFTGL])
])

METNO_REQUIRE_PKG([MILOGGER], [miLogger])
METNO_REQUIRE_PKG([PUCTOOLS], [puCtools])
METNO_REQUIRE_PKG([PUDATATYPES], [puDatatypes])
METNO_REQUIRE_PKG([PUTOOLS], [puTools])
AS_IF([test x$enable_batch_only != xyes], [
    METNO_REQUIRE_PKG([QUTILITIES], [qUtilities])
    METNO_REQUIRE_PKG([COSERVER], [coserver])
])

AS_IF([test x$enable_newarkobs = xyes], [
    METNO_REQUIRE_PKG([NEWARKAPI], [newarkAPI])
])

#
# Other third-party dependencies
#
METNO_REQUIRE_QT4([
    Core
    Gui
    Network
    Svg
    Xml
    XmlPatterns
])

AS_IF([test x$enable_embedded != xyes], [
    METNO_REQUIRE_QT4([
        OpenGL
    ])
])

AS_IF([test x$enable_batch_only != xyes], [
    METNO_REQUIRE_QT4([
    ])
])

METNO_REQUIRE_BOOST
METNO_REQUIRE_BOOST_FEATURE([regex])
METNO_REQUIRE_BOOST_FEATURE([system], [boost/system/api_config.hpp])

# libavformat for video production
AS_IF([test x$enable_video_export = xyes], [
    METNO_REQUIRE_C_LIBRARY([ZLIB], [z], [zlib.h], [zlibVersion])
    METNO_REQUIRE_AVFORMAT
])

METNO_REQUIRE_C_LIBRARY([PNG], [png], [png.h], [png_init_io], [${ZLIB_LIBS}])


# shapefile
METNO_REQUIRE_C_LIBRARY([SHP], [shp], [shapefil.h], [SHPOpen])

# libcurl - the multiprotocol file transfer library 
METNO_REQUIRE_C_LIBRARY([CURL], [curl], [curl/curl.h])

# EMOSLIB
AS_IF([test x$enable_obs_bufr = xyes], [
    METNO_REQUIRE_PKG([EMOS], [emos], [], [])
])

# udunits2
METNO_REQUIRE_C_LIBRARY([UU], [udunits2], [udunits2.h], [ut_new_system])

GTEST_CHECK([src/test/gtest])

#
# Output
#
AC_SUBST([program_suffix])

AC_CONFIG_FILES([bin/diana.bat])
AC_CONFIG_FILES([bin/diana-zip.bat])
AC_CONFIG_FILES([etc/diana/diana.setup])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([bin/Makefile])
AC_CONFIG_FILES([etc/Makefile])
AC_CONFIG_FILES([src/Makefile src/test/Makefile src/test/gtest/Makefile])
AC_CONFIG_FILES([perl/Makefile perl/Metno-Bdiana/lib/Metno/Bdiana.pm])
AC_CONFIG_FILES([share/applications/metno-diana-versioned.desktop])
AC_OUTPUT
