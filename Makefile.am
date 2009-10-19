# $Id$

ACLOCAL_AMFLAGS = -I m4

APP=diana

-include mk/install_test.mk
-include mk/debian.mk

SUBDIRS = src

bdir= $(bindir)
dist_b_SCRIPTS = bin/$(APP)

etcdir = $(sysconfdir)/$(APP)
dist_etc_DATA = etc/diana/diana.setup

pixmapsdir= $(datarootdir)/pixmaps
dist_pixmaps_DATA= share/pixmaps/$(APP).png 

applicationsdir= $(datarootdir)/applications
dist_applications_DATA= share/applications/metno-$(APP).desktop

include share/doc/diana/doc.mk
include share/diana/diana.mk
