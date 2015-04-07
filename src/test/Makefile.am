
diana_lib = $(top_builddir)/src/libdiana.a
CLEANFILES =

TESTS = 
check_PROGRAMS =
LDADD =

if HAVE_GTEST
TESTS += dianaUnitTests
check_PROGRAMS += dianaUnitTests dianaGuiTests

dianaUnitTests_qt_sources = \
    TestVcrossQtManager.cc \
    TestVcrossQuickmenues.cc

dianaUnitTests_moc_sources = $(dianaUnitTests_qt_sources:.cc=.moc.cc)

dianaUnitTests_SOURCES = \
    $(dianaUnitTests_qt_sources) \
    TestVcrossComputer.cc \
    TestVprofData.cc \
    TestCommandParser.cc \
    TestLogFileIO.cc \
    TestPlotOptions.cc \
    TestPoint.cc \
    TestUtilities.cc \
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

if !WITH_EMBEDDED
dianaUnitTests_SOURCES += \
    TestDianaLevels.cc
endif

LDFLAGS += -L.. \
    $(DIFIELD_LDFLAGS) \
    $(MIRASTER_LDFLAGS) \
    $(EMOS_LDFLAGS) \
    $(PUDATATYPES_LDFLAGS) \
    $(SHP_LDFLAGS) \
    $(GLTEXT_LDFLAGS) \
    $(GLU_LDFLAGS) \
    $(gtest_LDFLAGS) \
    $(PNG_LDFLAGS) \
    $(QT4_LDFLAGS)

CPPFLAGS += \
    $(AM_CPPFLAGS) -I$(srcdir)/.. \
    -DTEST_SRCDIR=\"$(abs_srcdir)\" -DTEST_BUILDDIR=\"$(abs_builddir)\" \
    $(DIFIELD_CPPFLAGS) \
    $(MIRASTER_CPPFLAGS) \
    $(EMOS_CPPFLAGS) \
    $(PUDATATYPES_CPPFLAGS) \
    $(SHP_CPPFLAGS) \
    $(GLTEXT_CPPFLAGS) \
    $(GLU_CPPFLAGS) \
    $(gtest_CFLAGS) \
    $(PNG_CPPFLAGS) \
    $(QT4_CPPFLAGS)

LDADD += \
    -lgtest $(diana_lib) \
    $(DIFIELD_LIBS) \
    $(MIRASTER_LIBS) \
    $(EMOS_LIBS) $(FORTRAN_LIBS) \
    $(PUDATATYPES_LIBS) \
    $(SHP_LIBS) \
    $(GLTEXT_LIBS) \
    $(GLU_LIBS) \
    $(gtest_LIBS) \
    $(PNG_LIBS) \
    $(QT4_LIBS) \
    $(BOOST_SYSTEM_LIBS)

endif # HAVE_GTEST

EXTRA_DIST = arome_vprof.nc bangladesh_vc.nc bangkok_sonde.nc
