# Diana - A Free Meteorological Visualisation Tool
#
# Copyright (C) 2017-2022 met.no
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
CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
INCLUDE(MetnoUtils)
INCLUDE(MetnoQtUtils)
INCLUDE(MetnoVersions)
INCLUDE(MetnoFindPackage)

METNO_CMAKE_SETUP()
METNO_PVERSION(3.57.12)

OPTION(ENABLE_DIANA_OMP "Use OpenMP" OFF)
OPTION(ENABLE_PERL "Build perl module" OFF)
OPTION(ENABLE_TESTS "Build and run tests" ON)
OPTION(ENABLE_NEWARKOBS "Enable NEWARK observations" OFF)
OPTION(ENABLE_GEOTIFF "Enable GeoTiff support" OFF)
OPTION(ENABLE_OBS_BUFR "Enable observation from BUFR files" ON)
OPTION(ENABLE_HDF5 "Enable HDF5 file support" OFF)
OPTION(ENABLE_GRPC "Enable gRPC services" OFF)

IF(ENABLE_DIANA_OMP)
  SET(HAVE_OPENMP TRUE)
  FIND_PACKAGE(OpenMP REQUIRED)
  SET(openmp_PACKAGE OpenMP::OpenMP_CXX)
ENDIF()

IF (ENABLE_TESTS)
  INCLUDE(MetnoGTest)
ENDIF (ENABLE_TESTS)

SET(THREADS_PREFER_PTHREAD_FLAG TRUE)
FIND_PACKAGE(Threads REQUIRED)

FIND_PACKAGE(Boost COMPONENTS date_time REQUIRED)
FIND_PACKAGE(yaml-cpp REQUIRED)

FIND_PACKAGE(mi-programoptions "2.0.0" REQUIRED)
FIND_PACKAGE(mi-fieldcalc "0.1.0" REQUIRED)
FIND_PACKAGE(fimex "1.2" REQUIRED)

FIND_PACKAGE(Qt5 REQUIRED COMPONENTS
  Gui
  LinguistTools
  Network
  OpenGL
  PrintSupport
  Svg
  Xml
  XmlPatterns
)

SET(PKGS_CORE
  Qt5::Network
  Qt5::PrintSupport # needed for svg export, see SvgFileSink.cc
  Qt5::Svg
  Qt5::Xml
  Qt5::XmlPatterns

  Boost::boost
  Boost::date_time # only used by FimexIO.cc
  libfimex
  mi-fieldcalc
  yaml-cpp

  ${openmp_PACKAGE}
)

SET(PKGS_UI
  Qt5::Gui
  Qt5::OpenGL
)

METNO_FIND_PACKAGE(milogger
  VERSION_MIN "6.0.4"
  PKGCONFIG_NAME metlibs-milogger
)
LIST(APPEND PKGS_CORE ${milogger_PACKAGE})

METNO_FIND_PACKAGE(putools
  VERSION_MIN "8.0.0"
  PKGCONFIG_NAME metlibs-putools
)
LIST(APPEND PKGS_CORE ${putools_PACKAGE})

METNO_FIND_PACKAGE(pudatatypes
  VERSION_MIN "6.0.0"
  PKGCONFIG_NAME metlibs-pudatatypes
)
LIST(APPEND PKGS_CORE ${pudatatypes_PACKAGE})

METNO_FIND_PACKAGE(libtiff
  PKGCONFIG_NAME libtiff-4
)
LIST(APPEND PKGS_CORE ${libtiff_PACKAGE})

METNO_FIND_PACKAGE(libpng
  VERSION_MIN "1.2"
  PKGCONFIG_NAME libpng
)
LIST(APPEND PKGS_CORE ${libpng_PACKAGE})

METNO_FIND_PACKAGE(libcurl
  VERSION_MIN "7"
  PKGCONFIG_NAME libcurl
)
LIST(APPEND PKGS_CORE ${libcurl_PACKAGE})

METNO_FIND_PACKAGE(proj
  PKGCONFIG_NAME "proj"
  LIBRARY_NAME "proj"
  INCLUDE_HDR "proj.h" "proj_api.h"
)
MESSAGE (STATUS "proj version='${proj_PACKAGE_VERSION}'")
IF (proj_PACKAGE_VERSION STREQUAL "")
  # unknown version
  IF ("${proj_HEADER}" STREQUAL "proj.h")
    SET(HAVE_PROJ_H 1)
  ENDIF()
ELSEIF (proj_PACKAGE_VERSION VERSION_LESS "4.6.0")
  # <4.6
  MESSAGE(SEND_ERROR "proj version '${proj_PACKAGE_VERSION}' is too old")
ELSEIF (${proj_PACKAGE_VERSION} VERSION_LESS "5.0.0")
  # >=4.6,<5.0
  # proj_api.h
ELSEIF (${proj_PACKAGE_VERSION} VERSION_LESS "6.2.0")
  # >=5.0,<6.2
  MESSAGE(SEND_ERROR "proj version '${proj_PACKAGE_VERSION}' not supported")
ELSEIF (${proj_PACKAGE_VERSION} VERSION_LESS "8.0.0")
  # >=6.2,<8.0
  # ok, proj_api.h
ELSE ()
  # >=8.0
  SET(HAVE_PROJ_H 1)
ENDIF()
LIST(APPEND PKGS_CORE ${proj_PACKAGE})

METNO_FIND_PACKAGE(shp
  PKGCONFIG_NAME shapelib
  LIBRARY_NAME "shp"
  INCLUDE_HDR "shapefil.h"
)
LIST(APPEND PKGS_CORE ${shp_PACKAGE})

METNO_FIND_PACKAGE(udunits2
  VERSION_MIN "2"
  PKGCONFIG_NAME udunits
  LIBRARY_NAME "udunits2"
  INCLUDE_HDR "udunits2.h"
)
LIST(APPEND PKGS_CORE ${udunits2_PACKAGE})

IF (ENABLE_GRPC)
  # see https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/CMakeLists.txt#L97

  SET (GRPC_MIN_VERSION "1.3.2")

  find_package(gRPC ${GRPC_MIN_VERSION} CONFIG)
  if (gRPC_FOUND)
    message(STATUS "Using gRPC ${gRPC_VERSION} from cmake config")

    set(protobuf_MODULE_COMPATIBLE TRUE)
    find_package(Protobuf CONFIG REQUIRED)
    message(STATUS "Using protobuf ${Protobuf_VERSION}")

    set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
    set(_REFLECTION gRPC::grpc++_reflection)
    if(CMAKE_CROSSCOMPILING)
      find_program(_PROTOBUF_PROTOC protoc)
    else()
      set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
    endif()

    set(_GRPC_GRPCPP gRPC::grpc++)
    if(CMAKE_CROSSCOMPILING)
      find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
    else()
      set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
    endif()

    SET(GRPC_LIBRARIES ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})
    SET(PROTOC ${_PROTOBUF_PROTOC})
    SET(PROTOC_GEN_GRPC ${_GRPC_CPP_PLUGIN_EXECUTABLE})
    SET(GRPC_VERSION ${gRPC_VERSION})
  ELSE()
    PKG_CHECK_MODULES(PC_GRPC REQUIRED IMPORTED_TARGET grpc++>=${GRPC_MIN_VERSION} protobuf)
    SET(GRPC_LIBRARIES PkgConfig::PC_GRPC Threads::Threads)
    SET(GRPC_VERSION ${PC_GRPC_grpc++_VERSION})
    message(STATUS "Using gRPC++ ${GRPC_VERSION} from pkg-config")
    find_program(_PROTOBUF_PROTOC protoc)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
  ENDIF()

  # changed header files in grpc 1.10.0, see https://github.com/grpc/grpc/releases/tag/v1.10.0
  IF (GRPC_VERSION VERSION_LESS "1.10")
    SET(DIANA_GRPC_INCLUDES_IN_GRPCPP 0)
  ELSE()
    SET(DIANA_GRPC_INCLUDES_IN_GRPCPP 1)
  ENDIF()
ENDIF (ENABLE_GRPC)

IF (ENABLE_GEOTIFF)
  SET (GEOTIFF_HDRS libgeotiff/geotiff.h geotiff/geotiff.h geotiff.h)
  METNO_FIND_PACKAGE(geotiff
    LIBRARY_NAME geotiff
    INCLUDE_HDR ${GEOTIFF_HDRS}
  )
  LIST(APPEND PKGS_CORE ${geotiff_PACKAGE})

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

  SET(GEOTIFF TRUE)
ENDIF ()

IF (ENABLE_HDF5)
  METNO_FIND_PACKAGE(hdf5
    PKGCONFIG_NAME hdf5-serial
    LIBRARY_NAME "hdf5_serial;hdf5"
    INCLUDE_HDR "hdf5.h"
  )
  LIST(APPEND PKGS_CORE ${hdf5_PACKAGE})

  SET(HDF5FILE TRUE)
ENDIF ()

IF (ENABLE_NEWARKOBS)
  METNO_FIND_PACKAGE(newark
    PKGCONFIG_NAME newarkAPI
  )
  LIST(APPEND PKGS_CORE ${newark_PACKAGE})

  SET(ROADOBS TRUE)
  SET(NEWARK_INC TRUE)
ENDIF ()

IF (ENABLE_OBS_BUFR)
  METNO_FIND_PACKAGE(emos
    CMAKE_NAME libemos
    CMAKE_TARGETS emos_shared
    PKGCONFIG_NAME emos
  )
  LIST(APPEND PKGS_CORE ${emos_PACKAGE})

  SET(BUFROBS TRUE)
ENDIF ()

# these are for libdianaui only
METNO_FIND_PACKAGE(coserver
  VERSION_MIN "3.0.0"
  PKGCONFIG_NAME metlibs-coserver-qt5
)
LIST(APPEND PKGS_UI ${coserver_PACKAGE})

METNO_FIND_PACKAGE(qutilities
  VERSION_MIN "8.0.0"
  PKGCONFIG_NAME metlibs-qutilities-qt5
)
LIST(APPEND PKGS_UI ${qutilities_PACKAGE})

METNO_FIND_PACKAGE(ftgl
  PKGCONFIG_NAME ftgl
)
LIST(APPEND PKGS_UI ${ftgl_PACKAGE})

METNO_FIND_PACKAGE(glu
  PKGCONFIG_NAME glu
)
LIST(APPEND PKGS_UI ${glu_PACKAGE})

# where libdiana...*so private shared libraries are installed
SET (LIBDIANA_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/diana/${PVERSION}")

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${LIBDIANA_DIR}" INSTALL_LIBDIR_IS_SYSTEM_DIR)
IF(${INSTALL_LIBDIR_IS_SYSTEM_DIR} EQUAL -1)
  LIST(APPEND CMAKE_INSTALL_RPATH "${LIBDIANA_DIR}")
ENDIF()

IF (ENABLE_TESTS)
  ENABLE_TESTING()
ENDIF (ENABLE_TESTS)

ADD_SUBDIRECTORY(src)
IF (ENABLE_TESTS)
  ADD_SUBDIRECTORY(test)
ENDIF (ENABLE_TESTS)
IF (ENABLE_PERL)
  ADD_SUBDIRECTORY(perl)
ENDIF (ENABLE_PERL)

ADD_SUBDIRECTORY(etc)
ADD_SUBDIRECTORY(share)

