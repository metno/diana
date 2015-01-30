# $Id$

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src bin etc perl

pixmapsdir= $(datarootdir)/pixmaps
dist_pixmaps_DATA= share/pixmaps/diana.png

applicationsdir= $(datarootdir)/applications
dist_applications_DATA= share/applications/metno-diana-versioned.desktop

install-data-hook:
	mv $(DESTDIR)/$(applicationsdir)/metno-diana-versioned.desktop $(DESTDIR)/$(applicationsdir)/metno-diana-@PVERSION@.desktop
	mv $(DESTDIR)/$(pixmapsdir)/diana.png $(DESTDIR)/$(pixmapsdir)/diana-@PVERSION@.png
	sed -r -i s/\<PVERSION\>/@PVERSION@/ $(DESTDIR)/$(prefix)/share/doc/diana-@PVERSION@/*.html

include share/doc/diana/doc.mk
include share/diana/diana.mk
