# -*- Makefile -*-

noinst_LTLIBRARIES = libmiRaster.la

libmiRaster_la_SOURCES = satimg.cc

libmiRaster_la_CPPFLAGS = -fPIC \
	${METLIBS_CPPFLAGS} \
	${PROJ_CPPFLAGS} \
	${GEOTIFF_CPPFLAGS} \
	${TIFF_CPPFLAGS}

libmiRaster_la_LDFLAGS = -fPIC
libmiRaster_la_LIBADD =

#if ENABLE_SHARED_LIBRARY
#libmiRaster_la_LDFLAGS += \
#	${METLIBS_LDFLAGS} \
#	${PROJ_LDFLAGS} \
#	${TIFF_LDFLAGS}
#
#libmiRaster_la_LIBADD += \
#	${METLIBS_LIBS} \
#	${PROJ_LIBS} \
#	${TIFF_LIBS}
#endif

# XXX needs revisiting - HDF5 is currently unconditional
if WITH_HDF5
libmiRaster_la_SOURCES += satimgh5.cc
libmiRaster_la_CPPFLAGS += -DHDF5 ${HDF5_CPPFLAGS} -D_REENTRANT
libmiRaster_la_LDFLAGS += ${HDF5_LDFLAGS}
libmiRaster_la_LIBADD += ${HDF5_LIBS}
endif

if WITH_GEOTIFF
libmiRaster_la_SOURCES += satgeotiff.cc ImageCache.cpp
libmiRaster_la_CPPFLAGS += -DGEOTIFF ${GEOTIFF_CPPFLAGS}
#libmiRaster_la_LDFLAGS += ${GEOTIFF_LDFLAGS} ${BOOST_THREAD_LDFLAGS}
#libmiRaster_la_LIBADD += ${GEOTIFF_LIBS} ${BOOST_THREAD_LIBS}
endif

noinst_HEADERS = satimgh5.h \
    ImageCache.h \
    satgeotiff.h \
    satimg.h \
    satimgh5_sattemp.h
