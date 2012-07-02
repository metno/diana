# $Id$

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src bin etc

pixmapsdir= $(datarootdir)/pixmaps
dist_pixmaps_DATA= share/pixmaps/diana.png

applicationsdir= $(datarootdir)/applications
dist_applications_DATA= share/applications/metno-diana-versioned.desktop

install-data-hook:
	cd $(DESTDIR)/$(applicationsdir) && mv metno-diana-versioned.desktop metno-diana-@PVERSION@.desktop
	cd $(DESTDIR)/$(pixmapsdir) && mv diana.png diana-@PVERSION@.png

include share/doc/diana/doc.mk
include share/diana/diana.mk
