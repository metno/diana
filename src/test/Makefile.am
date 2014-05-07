
diana_lib = $(top_builddir)/src/libdiana.a

TESTS = 

check_PROGRAMS = 

if HAVE_GTEST
TESTS += dianaUnitTests
check_PROGRAMS += dianaUnitTests

dianaUnitTests_SOURCES = \
    TestPlotOptions.cc

dianaUnitTests_LDFLAGS = -L.. \
    $(DIFIELD_LDFLAGS) $(gtest_LDFLAGS)

dianaUnitTests_CPPFLAGS = \
    $(AM_CPPFLAGS) -I$(srcdir)/.. -DTOP_SRCDIR=\"$(top_srcdir)\" \
    $(DIFIELD_CPPFLAGS) \
    $(gtest_CFLAGS)

dianaUnitTests_LDADD = \
    -lgtest_main $(diana_lib) \
    $(DIFIELD_LIBS) \
    $(gtest_LIBS)

endif # HAVE_GTEST
