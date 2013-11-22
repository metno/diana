
TESTS = dianaUnitTests TestSvgContouring

check_PROGRAMS = dianaUnitTests \
	SvgContouring

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

SvgContouring_SOURCES = \
    SvgContouring.cc

SvgContouring_LDFLAGS = -L..

SvgContouring_CPPFLAGS = \
    $(AM_CPPFLAGS) -I$(srcdir)/.. -DTOP_SRCDIR=\"$(top_srcdir)\"

SvgContouring_LDADD = \
    -ldiana
