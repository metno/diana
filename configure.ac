#
AC_PREREQ([2.59])
AC_INIT([diana], [3.43.5], [diana@met.no])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 foreign parallel-tests])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

METNO_PVERSION
METNO_PROGRAM_SUFFIX

AX_CXX_COMPILE_STDCXX_11([noext])

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
# C
#
AC_LANG_PUSH([C])

CFLAGS="$CFLAGS -O2 -pipe -g -Wall -Wextra -fPIC"
CXXFLAGS="$CXXFLAGS -msse2 -march=core2 -Wall -fPIC"
AC_SYS_LARGEFILE

AC_LANG_POP([C])

#
# C++
#
AC_LANG_PUSH([C++])

AC_CXX_HAVE_STD
AC_CXX_HAVE_STL

# Check compiler supporting openmp in C++ and C
AC_ARG_ENABLE(openmp, [  --enable-openmp         enable use of openmp [[default=no]]],[
  if test "$enableval" = yes; then
AX_OPENMP([AC_DEFINE(HAVE_OPENMP,1,[Define if OpenMP is enabled])
           CXXFLAGS="$CXXFLAGS $OPENMP_CXXFLAGS";CFLAGS="$CFLAGS $OPENMP_CXXFLAGS"],[])
  fi
], [])
AC_LANG_POP([C++])
AC_SUBST(OPENMP_CXXFLAGS)

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

AC_ARG_ENABLE([obs-bufr],
    [AS_HELP_STRING([--disable-obs-bufr], [disable observation buffers])],
    [], [enable_obs_bufr=no])
AM_CONDITIONAL([WITH_OBS_BUFR], [test x$enable_obs_bufr = xyes])

AC_ARG_ENABLE([qt5],
    [AS_HELP_STRING([--enable-qt5], [use Qt5])],
    [], [enable_qt5=yes])

AC_ARG_ENABLE([gui],
    [AS_HELP_STRING([--enable-gui], [also compile the gui version of diana])],
        [], [enable_gui=yes])
AM_CONDITIONAL([WITH_GUI], [test x$enable_gui = xyes])

AC_ARG_ENABLE([perl],
    [AS_HELP_STRING([--enable-perl], [enable perl module)])],
	    [], [enable_perl=no])
AM_CONDITIONAL([WITH_PERL], [test x$enable_perl = xyes])


METNO_CHECK_PKGS([FIMEX], [fimex-0.64 >= 0.64.0])
AS_IF([test "x$FIMEX_FOUND" = x], [
    METNO_REQUIRE_PKGS([FIMEX], [fimex >= 0.64.0])
])
METNO_REQUIRE_PKGS([METLIBS], [metlibs-milogger >= 6.0.0 metlibs-putools >= 7.1.0 metlibs-pudatatypes >= 6.0.0])


# for Qt5:
AS_IF([test x$enable_qt5 = xyes], [
  AS_IF([test x$enable_gui = xyes], [
      METNO_REQUIRE_PKGS([METLIBSUI], [metlibs-coserver-qt5 >= 3.0.0 metlibs-qutilities-qt5 >= 8.0.0 ftgl glu])
      METNO_REQUIRE_PKGS([QT4], [Qt5Widgets Qt5Gui Qt5PrintSupport Qt5OpenGL Qt5Network Qt5Svg Qt5Xml Qt5XmlPatterns])
  ],[
      METNO_REQUIRE_PKGS([QT4], [Qt5Core Qt5Network Qt5Svg Qt5Xml Qt5XmlPatterns])
  ])
  METNO_CHECK_QT5_TOOLS()
],[
  AS_IF([test x$enable_gui = xyes], [
      METNO_REQUIRE_PKGS([METLIBSUI], [metlibs-coserver >= 3.0.0 metlibs-qutilities >= 8.0.0 ftgl glu])
      METNO_REQUIRE_PKGS([QT4], [QtGui QtOpenGL QtNetwork QtSvg QtXml QtXmlPatterns])
  ],[
      METNO_REQUIRE_PKGS([QT4], [QtCore QtNetwork QtSvg QtXml QtXmlPatterns])
  ])
  METNO_CHECK_QT4_TOOLS()
])

#
# Other third-party dependencies
#
METNO_REQUIRE_BOOST
METNO_REQUIRE_BOOST_FEATURE([date_time])
METNO_REQUIRE_BOOST_FEATURE([system], [boost/system/api_config.hpp])

AS_IF([test x$enable_obs_bufr = xyes], [
    METNO_REQUIRE_PKG([EMOS], [emos], [], [])
])
AS_IF([test x$enable_newarkobs = xyes], [
    METNO_REQUIRE_PKG([NEWARKAPI], [newarkAPI])
])

METNO_REQUIRE_PKGS([PNG], [libpng >= 1.2])
METNO_REQUIRE_PKGS([CURL], [libcurl >= 7])
METNO_REQUIRE_PKGS([UDUNITS], [udunits >= 2])

METNO_REQUIRE_C_LIBRARY([SHP], [shp], [shapefil.h], [SHPOpen])
METNO_REQUIRE_C_LIBRARY([PROJ], [proj], [proj_api.h], [pj_init_plus])

METNO_WITH_LIBRARY([GEOTIFF], [geotiff], [geotiff], [yes])
METNO_IF_WITH([geotiff], [
  METNO_CHECK_PKG([TIFF], [libtiff-4], [], [])
  AS_IF([test x"$TIFF_FOUND" = x], [
    METNO_REQUIRE_C_LIBRARY([JPEG], [jpeg], [jpeglib.h], [jpeg_read_header])
    METNO_REQUIRE_C_LIBRARY([TIFF], [tiff], [tiff.h], [TIFFOpen], [${ZLIB_LIBS} ${JPEG_LIBS}])
  ])
  AC_CHECK_HEADERS(
    [libgeotiff/geotiff.h geotiff/geotiff.h geotiff.h], [
      METNO_REQUIRE_C_LIBRARY([GEOTIFF], [geotiff], [], [main], [${ZLIB_LIBS} ${TIFF_LIBS}])
      break
    ])
], [])
METNO_WITH_CONDITIONAL([GEOTIFF], [geotiff])

# hdf5
METNO_WITH_HDF5([no])
METNO_WITH_CONDITIONAL([HDF5], [hdf5])

GTEST_CHECK([test/gtest])

#
# Output
#
AC_SUBST([program_suffix])

AC_CONFIG_FILES([etc/diana/diana.setup])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([etc/Makefile])
AC_CONFIG_FILES([src/Makefile src/diField/Makefile src/miRaster/Makefile])
AC_CONFIG_FILES([test/Makefile test/gtest/Makefile])
AC_CONFIG_FILES([perl/Makefile perl/Metno-Bdiana/lib/Metno/Bdiana.pm])
AC_CONFIG_FILES([share/applications/metno-diana-versioned.desktop])
AC_OUTPUT
