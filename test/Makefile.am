
SUBDIRS = gtest .

diana_lib = $(top_builddir)/src/libdiana.la
CLEANFILES =

TESTS = 
check_PROGRAMS =
LDADD =

if HAVE_GTEST
TESTS += dianaUnitTests diFieldTest
check_PROGRAMS += dianaUnitTests dianaGuiTests diFieldTest

req_libs = $(top_builddir)/src/libdiana.la
DEPENDENCIES = $(req_libs)

dianaUnitTests_qt_sources = \
    TestVcrossQtManager.cc \
    TestVcrossQuickmenues.cc

dianaUnitTests_moc_sources = $(dianaUnitTests_qt_sources:.cc=.moc.cc)

dianaUnitTests_SOURCES = \
    $(dianaUnitTests_qt_sources) \
    TestDianaLevels.cc \
    TestVcrossComputer.cc \
    TestVprofData.cc \
    TestCommandParser.cc \
    TestLogFileIO.cc \
    TestPlotOptions.cc \
    TestPoint.cc \
    TestQuickMenues.cc \
    TestUtilities.cc \
    TestWebMap.cc \
    gtestMain.cc

dianaGuiTests_SOURCES = \
    TestVcrossGui.cc \
    gtestMainQt.cc

nodist_dianaUnitTests_SOURCES = \
     $(dianaUnitTests_moc_sources)
CLEANFILES += $(dianaUnitTests_moc_sources)

%.moc.cc: %.h
	@mkdir -p $(shell dirname $@)
	$(MOC4) $(DEFAULT_INCLUDES) -o $@ $<

CPPFLAGS += \
    $(AM_CPPFLAGS) -I$(top_srcdir)/src \
    -DTEST_SRCDIR=\"$(abs_srcdir)\" -DTEST_BUILDDIR=\"$(abs_builddir)\" \
    $(EMOS_CPPFLAGS) \
    $(METLIBS_CPPFLAGS) \
    $(FIMEX_CPPFLAGS) \
    $(SHP_CPPFLAGS) \
    $(gtest_CFLAGS) \
    $(PNG_CPPFLAGS) \
    $(QT4_CPPFLAGS)

LDFLAGS += \
    $(EMOS_LDFLAGS) \
    $(METLIBS_LDFLAGS) \
    $(FIMEX_LDFLAGS) \
    $(SHP_LDFLAGS) \
    $(gtest_LDFLAGS) \
    $(PNG_LDFLAGS) \
    $(QT4_LDFLAGS)

LDADD += \
    $(req_libs) \
    $(METLIBS_LIBS) \
    $(FIMEX_LIBS) \
    $(gtest_LIBS) \
    $(QT4_LIBS) \
    $(BOOST_SYSTEM_LIBS)

dianaGuiTests_CPPFLAGS = \
    $(CPPFLAGS) \
    $(METLIBSUI_CPPFLAGS)

dianaGuiTests_LDFLAGS = \
    $(LDFLAGS) \
    $(METLIBSUI_LDFLAGS)

dianaGuiTests_LDADD = \
    $(LDADD) \
    $(METLIBSUI_LIBS)

diFieldTest_SOURCES = \
    FieldFunctionsTest.cc \
    GridConverterTest.cc \
    gtestMain.cc

diFieldTest_LDFLAGS = \
    $(METLIBS_LDFLAGS) \
    $(FIMEX_LDFLAGS) \
    ${UDUNITS_LDFLAGS} \
    $(PROJ_LDFLAGS) \
    $(BOOST_LDFLAGS) \
    $(gtest_LDFLAGS)

diFieldTest_CPPFLAGS = \
    -I$(top_srcdir)/src/diField \
    $(METLIBS_CPPFLAGS) \
    $(FIMEX_CPPFLAGS) \
    ${UDUNITS_CPPFLAGS} \
    $(PROJ_CPPFLAGS) \
    $(BOOST_CPPFLAGS) \
    $(gtest_CPPFLAGS)

diFieldTest_LDADD = \
    $(req_libs) \
    $(METLIBS_LIBS) \
    $(FIMEX_LIBS) \
    $(BOOST_DATE_TIME_LIBS)  \
    $(gtest_LIBS)

if ENABLE_GRIBFILE
diFieldTest_LDADD += -lgrib_api
endif

# begin fimex options
diFieldTest_SOURCES += \
	DataReshapeTest.cc \
	FieldManagerTest.cc \
	FimexSourceTest.cc \
	MetConstantsTest.cc \
	RectangleTest.cc \
	ShapeTest.cc \
	TimeFilterTest.cc \
	VcrossDataTest.cc \
	VcrossUtilTest.cc
diFieldTest_CPPFLAGS += \
	-DFIMEX
# end fimex options

endif # HAVE_GTEST

EXTRA_DIST = arome_vprof.nc bangladesh_vc.nc bangkok_sonde.nc

EXTRA_DIST += test_fimexio_rw.nc hirlam_vct1.nc hirlam_vct2.nc arome.nc \
    arome_vc.nc hirlam_vc.nc simra_vc.nc simra_vc_dyn.nc emep_etna_dyn.nc \
    wavespec.nc
