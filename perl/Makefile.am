if WITH_PERL

PERLMAKEFILE  = Metno-Bdiana/Makefile
PERLDEPEND    = Metno-Bdiana/Makefile.PL Metno-Bdiana/lib/Metno/Bdiana.pm
PERLCONFIG    =   PREFIX="$(prefix)" \
				  INC="-Wall -I. -I../$(top_srcdir)/src" \
				  OBJECT="\$$(O_FILES) ../$(top_builddir)/src/.libs/*.o"

#if DEBUG
#PERLCONFIG    += OPTIMIZE=-g
#endif

$(PERLMAKEFILE): $(PERLDEPEND) Makefile.am
	-test ! -d Metno-Bdiana && cp -pr $(srcdir)/Metno-Bdiana . && \\
	   chmod -R u+w Metno-Bdiana && touch Metno-Bdiana/.distclean
	cd Metno-Bdiana && @PERL@ Makefile.PL $(PERLCONFIG)

all-local: $(PERLMAKEFILE)
	cd Metno-Bdiana && $(MAKE) $(AM_MAKEFLAGS)

clean-local: $(PERLMAKEFILE)
	cd Metno-Bdiana && $(MAKE) $(AM_MAKEFLAGS) clean

install-exec-local:
	cd Metno-Bdiana && $(MAKE) $(AM_MAKEFLAGS) DESTDIR=$(DESTDIR) install

uninstall-local:
	rm -f $(DESTDIR)$(prefix)/lib/*/site_perl/*/*/Metno/Bdiana.pm
	rm -f $(DESTDIR)$(prefix)/lib/*/site_perl/*/*/auto/Metno/Bdiana/Bdiana.so
	rm -f $(DESTDIR)$(prefix)/lib/*/site_perl/*/*/auto/Metno/Bdiana/Bdiana.bs
	rm -f $(DESTDIR)$(prefix)/lib/*/site_perl/*/*/auto/Metno/Bdiana/autosplit.ix
	rm -f $(DESTDIR)$(prefix)/lib/*/site_perl/*/*/auto/Metno/Bdiana/.packlist
	rm -f $(DESTDIR)$(prefix)/lib/*/*/*/perllocal.pod
	rm -f $(DESTDIR)$(prefix)/share/man/man3/Metno::Bdiana.3

distclean-local:
	test -f Metno-Bdiana/.distclean && rm -fr Metno-Bdiana

endif


EXTRA_DIST = \
        Metno-Bdiana/ppport.h \
        Metno-Bdiana/Bdiana.xs \
        Metno-Bdiana/lib/Metno/Bdiana.pm \
        Metno-Bdiana/Makefile.PL \
        Metno-Bdiana/README \
        Metno-Bdiana/HACKS \
        Metno-Bdiana/t/Metno-Bdiana.t \
        Metno-Bdiana/Changes \
        Metno-Bdiana/MANIFEST \
        Metno-Bdiana/control
