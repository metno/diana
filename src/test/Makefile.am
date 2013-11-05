
TESTS = dianaUnitTests

check_PROGRAMS = dianaUnitTests

dianaUnitTests_SOURCES = \
    TestPlotOptions.cc

dianaUnitTests_LDFLAGS = -L.. \
    $(DIFIELD_LDFLAGS)

dianaUnitTests_CPPFLAGS = \
    $(AM_CPPFLAGS) -I$(srcdir)/.. -DTOP_SRCDIR=\"$(top_srcdir)\" \
    $(DIFIELD_CPPFLAGS) \
    $(gtest_CFLAGS)

dianaUnitTests_LDADD = \
    -lgtest_main -ldiana \
    $(DIFIELD_LIBS) \
    $(gtest_LIBS)
