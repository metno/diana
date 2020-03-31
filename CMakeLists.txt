# Diana - A Free Meteorological Visualisation Tool
#
# Copyright (C) 2017-2018 met.no
#
# Contact information:
# Norwegian Meteorological Institute
# Box 43 Blindern
# 0313 OSLO
# NORWAY
# email: diana@met.no
#
# This file is part of Diana
#
# Diana is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Diana is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Diana; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

PROJECT(diana)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
INCLUDE(MetnoUtils)
INCLUDE(MetnoGTest)
INCLUDE(MetnoQtUtils)
INCLUDE(MetnoVersions)

METNO_CMAKE_SETUP()
METNO_PVERSION(3.52.5)

OPTION(ENABLE_DIANA_OMP "Use OpenMP" OFF)
IF(ENABLE_DIANA_OMP)
  FIND_PACKAGE(OpenMP)
  IF(OPENMP_FOUND)
    SET(HAVE_OPENMP TRUE)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  ENDIF()
ENDIF()

OPTION(ENABLE_PERL "Build perl module" OFF)
OPTION(ENABLE_SMHI "Build SMHI variant" OFF)
OPTION(ENABLE_NEWARKOBS "Enable NEWARK observations" OFF)
OPTION(ENABLE_GEOTIFF "Enable GeoTiff support" OFF)
OPTION(ENABLE_OBS_BUFR "Enable observation from BUFR files" ON)
OPTION(ENABLE_HDF5 "Enable HDF5 file support" OFF)


FIND_PACKAGE(Boost COMPONENTS date_time system REQUIRED)

FIND_PACKAGE(mi-fieldcalc "0.1.0" REQUIRED)
FIND_PACKAGE(fimex "1.2" REQUIRED)

METNO_FIND_QT(Gui Network OpenGL PrintSupport Svg Xml XmlPatterns)

SET(PKGS
  metlibs-milogger>=6.0.0
  metlibs-putools>=8.0.0
  metlibs-pudatatypes>=6.0.0

  libtiff-4
  libpng>=1.2
  libcurl>=7
)

IF (ENABLE_GEOTIFF)
  SET (GEOTIFF_HDRS libgeotiff/geotiff.h geotiff/geotiff.h geotiff.h)
  METNO_FIND_PACKAGE("geotiff" "" "geotiff" "${GEOTIFF_HDRS}")
  LIST(APPEND PKGS ${geotiff_PC})

  UNSET (HAVE_LIBGEOTIFF_GEOTIFF_H CACHE)
  UNSET (HAVE_GEOTIFF_GEOTIFF_H CACHE)
  UNSET (HAVE_GEOTIFF_H CACHE)
  IF ("${geotiff_HEADER}" STREQUAL "libgeotiff/geotiff.h")
    SET (HAVE_LIBGEOTIFF_GEOTIFF_H TRUE)
  ELSEIF ("${geotiff_HEADER}" STREQUAL "geotiff/geotiff.h")
    SET (HAVE_GEOTIFF_GEOTIFF_H TRUE)
  ELSEIF ("${geotiff_HEADER}" STREQUAL "geotiff.h")
    SET (HAVE_GEOTIFF_H TRUE)
  ELSE ()
    MESSAGE (FATAL_ERROR "Unsupported geotiff.h subdirectory: '${geotiff_HEADER}'")
  ENDIF ()
ENDIF ()

IF (ENABLE_HDF5)
  METNO_FIND_PACKAGE("hdf5" "hdf5-serial" "hdf5_serial;hdf5" "hdf5.h")
  LIST(APPEND PKGS ${hdf5_PC})
ENDIF ()

METNO_FIND_PACKAGE("proj" "proj" "proj" "proj_api.h")
LIST(APPEND PKGS ${proj_PC})

METNO_FIND_PACKAGE("shp" "" "shp" "shapefil.h")
LIST(APPEND PKGS ${shp_PC})

METNO_FIND_PACKAGE("udunits2" "udunits>=2" "udunits2" "udunits2.h")
LIST(APPEND PKGS ${udunits2_PC})

IF (ENABLE_NEWARKOBS)
  LIST(APPEND PKGS newarkAPI)
ENDIF ()
IF (ENABLE_OBS_BUFR)
  LIST(APPEND PKGS emos)
ENDIF ()

# these are for libdianaui only
SET(PKGS_UI
  metlibs-coserver-qt5>=3.0.0
  metlibs-qutilities-qt5>=8.0.0
  ftgl
  glu
)
PKG_CHECK_MODULES(PC   REQUIRED ${PKGS})
PKG_CHECK_MODULES(PCUI REQUIRED ${PKGS_UI})

# where libdiana...*so private shared libraries are installed
SET (LIBDIANA_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/diana/${PVERSION}")

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${LIBDIANA_DIR}" INSTALL_LIBDIR_IS_SYSTEM_DIR)
IF(${INSTALL_LIBDIR_IS_SYSTEM_DIR} EQUAL -1)
  LIST(APPEND CMAKE_INSTALL_RPATH "${LIBDIANA_DIR}")
ENDIF()

ENABLE_TESTING()

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(perl)
ADD_SUBDIRECTORY(test)

ADD_SUBDIRECTORY(etc)
ADD_SUBDIRECTORY(share)
