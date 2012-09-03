# $Id$

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src bin etc

pixmapsdir= $(datarootdir)/pixmaps
dist_pixmaps_DATA= share/pixmaps/diana.png

applicationsdir= $(datarootdir)/applications
dist_applications_DATA= share/applications/metno-diana-versioned.desktop \
	share/applications/metno-diana.desktop

install-data-hook:
	mv $(DESTDIR)/$(applicationsdir)/metno-diana-versioned.desktop $(DESTDIR)/$(applicationsdir)/metno-diana-@PVERSION@.desktop
	cp $(DESTDIR)/$(pixmapsdir)/diana.png $(DESTDIR)/$(pixmapsdir)/diana-@PVERSION@.png # not mv!
	if test -n "@PSUFFIX@"; then \
	    mv $(DESTDIR)/$(pixmapsdir)/diana.png $(DESTDIR)/$(pixmapsdir)/diana@PSUFFIX@.png ; \
	    mv $(DESTDIR)/$(applicationsdir)/metno-diana.desktop $(DESTDIR)/$(applicationsdir)/metno-diana@PSUFFIX@.desktop ; \
	fi

include share/doc/diana/doc.mk
include share/diana/diana.mk
