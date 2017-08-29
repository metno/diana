
SUBDIRS = gtest .

diana_lib = $(top_builddir)/src/libdiana.la
CLEANFILES =

TESTS = 
check_PROGRAMS =
LDADD =

if HAVE_GTEST
TESTS += dianaUnitTests diFieldTest
check_PROGRAMS += dianaUnitTests diFieldTest
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
    TestPlotCommands.cc \
    TestPlotOptions.cc \
    TestPoint.cc \
    TestQuickMenues.cc \
    TestSatImg.cc \
    TestSetupParser.cc \
    TestUtilities.cc \
    TestWebMap.cc \
    gtestMainQCA.cc

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
    $(gtest_CPPFLAGS) \
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

diFieldTest_SOURCES = \
    FieldFunctionsTest.cc \
    GridConverterTest.cc \
    ProjectionTest.cc \
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

# begin fimex options
diFieldTest_SOURCES += \
	DataReshapeTest.cc \
	FieldManagerTest.cc \
	FimexSourceTest.cc \
	MetConstantsTest.cc \
	RectangleTest.cc \
	ShapeTest.cc \
	VcrossDataTest.cc \
	VcrossUtilTest.cc
diFieldTest_CPPFLAGS += \
	-DFIMEX
# end fimex options

if WITH_GUI
check_PROGRAMS += dianaGuiTests
req_ui_libs = $(top_builddir)/src/libdianaui.a
DEPENDENCIES += $(req_ui_libs)

dianaGuiTests_SOURCES = \
    TestVcrossGui.cc \
    TestGLPainter.cc \
    gtestMainQA.cc

dianaGuiTests_CPPFLAGS = \
    $(CPPFLAGS) \
    $(METLIBS_CPPFLAGS) \
    $(METLIBSUI_CPPFLAGS)

dianaGuiTests_LDFLAGS = \
    $(LDFLAGS) \
    $(METLIBS_LDFLAGS) \
    $(METLIBSUI_LDFLAGS)

dianaGuiTests_LDADD = \
    $(LDADD) \
    $(req_ui_libs) \
    $(METLIBS_LIBS) \
    $(METLIBSUI_LIBS)
endif # WITH_GUI


endif # HAVE_GTEST

EXTRA_DIST = arome_vprof.nc bangladesh_vc.nc bangkok_sonde.nc

EXTRA_DIST += test_fimexio_rw.nc hirlam_vct1.nc hirlam_vct2.nc arome.nc \
    arome_vc.nc hirlam_vc.nc simra_vc.nc simra_vc_dyn.nc emep_etna_dyn.nc \
    wavespec.nc

EXTRA_DIST += arome_smhi.nc approach_ENHF.kml

all-local: $(check_PROGRAMS)

# see https://lists.gnu.org/archive/html/automake/2013-06/msg00051.html
display-testsuite-logs:
	for f in $(TEST_LOGS); do printf "\n=== $$f ===\n\n"; cat $$f; done
.PHONY: display-testsuite-logs
