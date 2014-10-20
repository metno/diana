
diana_lib = $(top_builddir)/src/libdiana.a

TESTS = 

check_PROGRAMS = 

if HAVE_GTEST
TESTS += dianaUnitTests
check_PROGRAMS += dianaUnitTests

dianaUnitTests_SOURCES = \
    TestVprofData.cc \
    TestPlotOptions.cc

if !WITH_EMBEDDED
dianaUnitTests_SOURCES += \
    TestDianaLevels.cc
endif

dianaUnitTests_LDFLAGS = -L.. \
    $(DIFIELD_LDFLAGS) \
    $(PUDATATYPES_LDFLAGS) \
    $(GLTEXT_LDFLAGS) \
    $(GLU_LDFLAGS) \
    $(gtest_LDFLAGS)

dianaUnitTests_CPPFLAGS = \
    $(AM_CPPFLAGS) -I$(srcdir)/.. -DTOP_SRCDIR=\"$(top_srcdir)\" \
    $(DIFIELD_CPPFLAGS) \
    $(PUDATATYPES_CPPFLAGS) \
    $(GLTEXT_CPPFLAGS) \
    $(GLU_CPPFLAGS) \
    $(gtest_CFLAGS)

dianaUnitTests_LDADD = \
    -lgtest_main $(diana_lib) \
    $(DIFIELD_LIBS) \
    $(PUDATATYPES_LIBS) \
    $(GLTEXT_LIBS) \
    $(GLU_LIBS) \
    $(gtest_LIBS)

endif # HAVE_GTEST

EXTRA_DIST = arome_vprof.nc
