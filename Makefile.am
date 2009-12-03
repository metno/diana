# $Id$

ACLOCAL_AMFLAGS = -I m4


if ENABLE_BETA
APP=diana@program_suffix@
else
APP=diana
endif

-include mk/debian.mk

SUBDIRS = src

bdir= $(bindir)
dist_b_SCRIPTS = bin/diana

etcdir = $(sysconfdir)/$(APP)
dist_etc_DATA = etc/diana/diana.setup

pixmapsdir= $(datarootdir)/pixmaps
dist_pixmaps_DATA= share/pixmaps/$(APP).png 

applicationsdir= $(datarootdir)/applications
dist_applications_DATA= share/applications/metno-$(APP).desktop


include share/doc/diana/doc.mk
include share/diana/diana.mk
