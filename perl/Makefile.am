
if WITH_PERL
PERL          = perl
PERLMAKEFILE  = Metno-Bdiana/Makefile
PERLDEPEND    = $(srcdir)/Metno-Bdiana/Makefile.PL Metno-Bdiana/lib/Metno/Bdiana.pm
PERLCONFIG    = PREFIX="$(prefix)" \
                INSTALLDIRS=vendor \
                INC="-Wall -I. -I$(abs_top_srcdir)/src" \
                LIBS="$(top_builddir)/src/libdiana.la -lboost_thread"

#if DEBUG
#PERLCONFIG    += OPTIMIZE=-g
#endif

PERLFILES = Metno-Bdiana/control Metno-Bdiana/Bdiana.xs Metno-Bdiana/ppport.h Metno-Bdiana/Makefile.PL

$(PERLMAKEFILE): $(PERLDEPEND) Makefile.am $(PERLFILES:%=$(srcdir)/%)
	for p in $(PERLFILES); do if test "$(srcdir)/$$p" -nt "$$p"; then cp -vf "$(srcdir)/$$p" "$$p" ; fi ; done
	cd Metno-Bdiana && $(PERL) $(abs_srcdir)/Metno-Bdiana/Makefile.PL $(PERLCONFIG)

all-local: $(PERLMAKEFILE) $(PERLFILES)
	$(MAKE) -C Metno-Bdiana $(AM_MAKEFLAGS)

clean-local: $(PERLMAKEFILE)
	$(MAKE) -C Metno-Bdiana $(AM_MAKEFLAGS) clean

if WITH_GUI
check-local: $(PERLMAKEFILE)
	$(MAKE) -C Metno-Bdiana $(AM_MAKEFLAGS) test
endif

install-exec-local:
	$(MAKE) -C Metno-Bdiana $(AM_MAKEFLAGS) DESTDIR=$(DESTDIR) install

uninstall-local:
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/Metno/Bdiana.pm
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/auto/Metno/Bdiana/Bdiana.so
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/auto/Metno/Bdiana/Bdiana.bs
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/auto/Metno/Bdiana/autosplit.ix
	rm -f $(DESTDIR)$(prefix)/lib/*/perl5/*/*/auto/Metno/Bdiana/.packlist
	rm -f $(DESTDIR)$(prefix)/lib/*/*/*/perllocal.pod
	rm -f $(DESTDIR)$(prefix)/share/man/man3/Metno::Bdiana.3

endif # WITH_PERL

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
