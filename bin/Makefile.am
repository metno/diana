# $Id$


if WIN32
bin_SCRIPTS = diana.bat diana-zip.bat
# On Win32, to simplify installation, copy Qt libraries into $bindir
install-exec-local:
	@for dll in ${QT4_DLL} ; do \
	    if [ ! -f "${bindir}/$${dll}" ] ; then \
		echo "Copying $${dll} to ${bindir}" ;  \
		${INSTALL_PROGRAM} ${QT4_DIR}/bin/$${dll} ${bindir} ; \
	    fi ; \
	done
endif
