# $Id$

ACLOCAL_AMFLAGS = -I m4

-include mk/debian.mk

SUBDIRS = src bin etc

pixmapsdir= $(datarootdir)/pixmaps
dist_pixmaps_DATA= share/pixmaps/$(APP).png 

applicationsdir= $(datarootdir)/applications
dist_applications_DATA= share/applications/metno-$(APP).desktop


include share/doc/diana/doc.mk
include share/diana/diana.mk
