# automake requires one = before the first +=
common_ldflags = -L$(top_srcdir)/../src/
common_ldadd = -ldiana

if WITH_BATCH_ONLY
common_ldflags += 
else 
common_ldflags += \
	$(QUTILITIES_LDFLAGS)

common_ldadd += \
	$(QUTILITIES_LIBS)
endif

common_ldflags += \
	$(DIFIELD_LDFLAGS) \
	$(DIMITIFF_LDFLAGS) \
	$(PUCTOOLS_LDFLAGS) \
	$(EMOS_LDFLAGS) \
	$(PUDATATYPES_LDFLAGS) \
	$(ROBS_LDFLAGS) \
	$(SHP_LDFLAGS) \
	$(CURL_LDFLAGS) \
	$(PNG_LDFLAGS) \
        $(UU_LDFLAGS)

common_ldadd += \
	$(DIFIELD_LIBS) \
	$(DIMITIFF_LIBS) \
	$(PUCTOOLS_LIBS) \
	$(EMOS_LIBS) \
	$(PUDATATYPES_LIBS) \
	$(ROBS_LIBS) \
	$(SHP_LIBS) \
	$(CURL_LIBS) \
	$(PNG_LIBS) \
        $(UU_LIBS)

if WITH_ROADOBS
common_ldflags += \
	${ROADAPI_LDFLAGS}
common_ldadd += \
	${ROADAPI_LIBS}
endif

if WITH_NEWARKOBS
common_ldflags += \
	${NEWARKAPI_LDFLAGS}
common_ldadd += \
	${NEWARKAPI_LIBS}
endif

if WITH_GRIBFILE
common_ldflags += \
	${GRIBAPI_LDFLAGS}
common_ldadd += \
	${GRIBAPI_LIBS} ${GRIBAPI_LIBS}
endif

# Additional source code for RoadOBS support
if WITH_GEOTIFF
common_ldadd += \
	${ROADAPI_LIBS} ${GRIBAPI_LIBS}
endif

# Additional source code for met.no prod db
if WITH_PRODDB
common_ldflags += \
	$(PUSQL_LDFLAGS) \
	$(DISQL_LDFLAGS)
common_ldadd += \
	$(PUSQL_LIBS) \
	$(DISQL_LIBS)
endif

if WITH_PROFET
common_ldflags += \
	$(PROFET_LDFLAGS)
common_ldadd += \
	$(PROFET_LIBS)
endif

# Additional source code for video export feature
if WITH_VIDEO_EXPORT
common_ldflags += $(AVFORMAT_LDFLAGS)
common_ldadd += $(AVFORMAT_LIBS)
endif

# Embedded build support - remove X and OpenGL dependencies
if WITH_EMBEDDED
common_ldflags += 
else
common_ldflags += \
        $(FTGL_LDFLAGS) \
        $(GLP_LDFLAGS) \
        $(GLTEXT_LDFLAGS) \
        $(GLU_LDFLAGS)

common_ldadd += \
        $(FTGL_LIBS) \
        $(GLP_LIBS) \
        $(GLTEXT_LIBS) \
        $(GLU_LIBS)
endif

common_ldflags += $(QT4_LDFLAGS)
common_ldadd += $(QT4_LIBS)

# Win32 support
common_ldflags += $(WIN32_LDFLAGS)
common_ldadd += $(WIN32_LIBS)



if WITH_PERL
PERL          = perl
PERLMAKEFILE  = Metno-Bdiana/Makefile
PERLDEPEND    = Metno-Bdiana/Makefile.PL Metno-Bdiana/lib/Metno/Bdiana.pm
PERLCONFIG    =   PREFIX="$(prefix)" \
				  INSTALLDIRS=vendor \
				  INC="-Wall -I. -I$(top_srcdir)/../src" \
				  LIBS="-L$(top_srcdir)/../src/ $(common_ldflags) $(common_ldadd)"

#if DEBUG
#PERLCONFIG    += OPTIMIZE=-g
#endif

$(PERLMAKEFILE): $(PERLDEPEND) Makefile.am
	-test ! -d Metno-Bdiana && cp -pr $(srcdir)/Metno-Bdiana . && \\
	   chmod -R u+w Metno-Bdiana && touch Metno-Bdiana/.distclean
	cd Metno-Bdiana && $(PERL) Makefile.PL $(PERLCONFIG)

all-local: $(PERLMAKEFILE)
	cd Metno-Bdiana && $(MAKE) $(AM_MAKEFLAGS)

clean-local: $(PERLMAKEFILE)
	cd Metno-Bdiana && $(MAKE) $(AM_MAKEFLAGS) clean

check-local: $(PERLMAKEFILE)
	cd Metno-Bdiana && $(MAKE) $(AM_MAKEFLAGS) test

install-exec-local:
	cd Metno-Bdiana && $(MAKE) $(AM_MAKEFLAGS) DESTDIR=$(DESTDIR) install

uninstall-local:
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/Metno/Bdiana.pm
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/auto/Metno/Bdiana/Bdiana.so
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/auto/Metno/Bdiana/Bdiana.bs
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/auto/Metno/Bdiana/autosplit.ix
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/auto/Metno/Bdiana/.packlist
	rm -f $(DESTDIR)$(prefix)/lib/*/*/*/perllocal.pod
	rm -f $(DESTDIR)$(prefix)/share/man/man3/Metno::Bdiana.3

distclean-local:
	test -f Metno-Bdiana/.distclean
	rm -f Makefile
	rm -f Makefile.in
	cd Metno-Bdiana && $(MAKE) $(AM_MAKEFLAGS) distclean

endif


EXTRA_DIST = \
        Metno-Bdiana/ppport.h \
        Metno-Bdiana/Bdiana.xs \
        Metno-Bdiana/lib/Metno/Bdiana.pm.in \
        Metno-Bdiana/Makefile.PL \
        Metno-Bdiana/README \
        Metno-Bdiana/t/Metno-Bdiana.t \
        Metno-Bdiana/Changes \
        Metno-Bdiana/MANIFEST \
        Metno-Bdiana/control
