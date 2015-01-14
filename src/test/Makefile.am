
diana_lib = $(top_builddir)/src/libdiana.a

TESTS = 

check_PROGRAMS =

if HAVE_GTEST
TESTS += dianaUnitTests
check_PROGRAMS += dianaUnitTests

dianaUnitTests_qt_sources = \
    TestVcrossQtManager.cc \
    TestVcrossQuickmenues.cc

dianaUnitTests_moc_sources = $(dianaUnitTests_qt_sources:.cc=.moc.cc)

dianaUnitTests_SOURCES = \
    $(dianaUnitTests_qt_sources) \
    TestVprofData.cc \
    TestCommandParser.cc \
    TestLogFileIO.cc \
    TestPlotOptions.cc \
    TestUtilities.cc

nodist_dianaUnitTests_SOURCES = \
     $(dianaUnitTests_moc_sources)

%.moc.cc: %.h
	@mkdir -p $(shell dirname $@)
	$(MOC4) $(DEFAULT_INCLUDES) -o $@ $<

if !WITH_EMBEDDED
dianaUnitTests_SOURCES += \
    TestDianaLevels.cc
endif

dianaUnitTests_LDFLAGS = -L.. \
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

dianaUnitTests_CPPFLAGS = \
    $(AM_CPPFLAGS) -I$(srcdir)/.. -DTOP_SRCDIR=\"$(top_srcdir)\" \
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

dianaUnitTests_LDADD = \
    -lgtest_main $(diana_lib) \
    $(DIFIELD_LIBS) \
    $(MIRASTER_LIBS) \
    $(EMOS_LIBS) $(FORTRAN_LIBS) \
    $(PUDATATYPES_LIBS) \
    $(SHP_LIBS) \
    $(GLTEXT_LIBS) \
    $(GLU_LIBS) \
    $(gtest_LIBS) \
    $(PNG_LIBS) \
    $(QT4_LIBS)

endif # HAVE_GTEST

EXTRA_DIST = arome_vprof.nc
