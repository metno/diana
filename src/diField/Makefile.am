# -*- Makefile -*-

noinst_LTLIBRARIES = libdiField.la

libdiField_la_SOURCES = \
	DataReshape.cc \
	diArea.cc \
	diField.cc \
	diFieldFunctions.cc \
	diFieldManager.cc \
	diGridConverter.cc \
	diMetConstants.cc \
	diProjection.cc \
	diRectangle.cc\
	diFieldCacheKeyset.cc \
	diFieldCacheEntity.cc \
	diFieldCache.cc \
	GridInventoryTypes.cc \
	GridIO.cc \
	GridCollection.cc \
	TimeFilter.cc \
	FimexSource.cc \
	VcrossSource.cc \
	VcrossData.cc \
	VcrossUtil.cc

libdiField_la_CPPFLAGS = -fPIC \
	${BOOST_CPPFLAGS} \
	${PROJ_CPPFLAGS} \
	${UDUNITS_CPPFLAGS} \
	${METLIBS_CPPFLAGS}

# these are only for shared libraries
#libdiField_la_LDFLAGS =
#libdiField_la_LIBADD =


noinst_HEADERS = \
	diArea.h \
	diCommonFieldTypes.h \
	diField.h \
	diFieldExceptions.h \
	diFieldFunctions.h \
	diFieldManager.h \
	diGridConverter.h \
	diMetConstants.h \
	diProjection.h \
	diRectangle.h \
	diFieldCache.h \
	diFieldCacheKeyset.h \
	diFieldCacheEntity.h \
	GridInventoryTypes.h \
	GridDataKey.h \
	GridIO.h \
	GridCollection.h \
	TimeFilter.h \
	FimexSource.h \
	VcrossSource.h \
	VcrossData.h \
	VcrossUtil.h \
	DataReshape.h

# begin fimex options
libdiField_la_SOURCES += \
	FimexIO.cc
libdiField_la_CPPFLAGS += \
	-DFIMEX \
	${FIMEX_CPPFLAGS}
# these are only for shared libraries
#libdiField_la_LDFLAGS += \
#	${FIMEX_LDFLAGS}
#libdiField_la_LIBADD += \
#	${FIMEX_LIBS}

noinst_HEADERS += FimexIO.h
# end fimex options
