# $Id$

ACLOCAL_AMFLAGS = -I m4

APP=diana

include mk/install_test.mk

SUBDIRS = src

bdir= $(bindir)
dist_b_SCRIPTS = bin/diana 

etcdir = $(sysconfdir)/$(APP)
dist_etc_DATA = etc/diana/diana.setup

pixmapsdir= $(datarootdir)/pixmaps
dist_pixmaps_DATA= share/pixmaps/diana.png \
	share/pixmaps/dianaTEST.png \
	share/pixmaps/dianaPROFF.png

applicationsdir= $(datarootdir)/applications
dist_applications_DATA= share/applications/metno-diana.desktop

include share/doc/diana/doc.mk
include share/diana/diana.mk
