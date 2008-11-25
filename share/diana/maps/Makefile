# Makfile to install files in the /metno/local/maps/ directories elsewhere
#

DESTDIR= /usr/local/stow
DESTNAME= diana_maps-20081125

def:
	@echo 'usage: make [ install DESTDIR=<dir> ]'

install:
	  mkdir -pv $(DESTDIR)/$(DESTNAME)/; \
	  for FILE in `ls -A1 -I *~ -I Makefile -I .svn`; do \
	    cp -upv $$FILE $(DESTDIR)/$(DESTNAME)/ ; \
	done; 


