# Makfile to install config files 
#

DESTDIR= /usr/local/stow/diana_config-20081125/etc/diana

def:
	@echo 'usage: make [ install DESTDIR=<dir> ]'

install:
	  mkdir -pv $(DESTDIR)/; \
	  for FILE in `ls -A1 -I *~ -I Makefile -I .svn`; do \
	    cp -upv $$FILE $(DESTDIR)/ ; \
	done; 


