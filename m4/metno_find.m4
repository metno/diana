# -*- autoconf -*-
#
# $Id: metno_find.m4 2766 2009-09-14 04:44:08Z dages $
#
# SYNOPSIS
#
#   METNO_FIND_HEADER(variable-prefix, header, subdirs)
#   METNO_FIND_LIBRARY(variable-prefix, library, subdirs)
#
# DESCRIPTION
#
#   The METNO_FIND_HEADER macro attempts to locate the specified
#   header file.  If it succeeds, it sets <variable-prefix>_INCLUDEDIR
#   to the name of the directory where the header was found.  If a
#   <variable-prefix>_INCLUDEDIR variable exists and is set, it will
#   respect it.
#
#   The METNO_FIND_LIBRARY macro attempts to locate the specified
#   library.  If it succeeds, it sets <variable-prefix>_LIBDIR to the
#   name of the directory where the header was found.  If a
#   <variable-prefix>_LIBDIR variable exists and is set, it will
#   respect it.
#
#   In both cases, the third argument is a (potentially empty) list of
#   subdirectories where the header or library may or may not be
#   located.  For instance,
#
#   METNO_FIND_HEADER([foo], [foo.h], [bar])
#
#   will look for /usr/include/foo.h, /usr/include/bar/foo.h etc.
#
#   Note that in both cases, if there are multiple alternatives, the
#   one these macros find is not necessarily the one the toolchain
#   will choose.
#
# TODO
#
#   Improved documentation.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

m4_define([METNO_FIND_DIRS], [/usr/local /usr])

#
# METNO_FIND_HEADER
#
# $1 = variable-prefix
# $2 = header
# $3 = subdirs
#
AC_DEFUN([METNO_FIND_HEADER], [
    if test x"${$1_INCLUDEDIR}" = x ; then
	for DIR in METNO_FIND_DIRS $prefix ; do
	    for SUFFIX in include \
	        m4_foreach([SUBDIR], [$3], [include/SUBDIR SUBDIR/include]) ; do
		if test -r ${DIR}/${SUFFIX}/$2 ; then
		    $1_INCLUDEDIR=${DIR}/${SUFFIX}
		    break 2
		fi
	    done
	done
    fi
])

#
# METNO_FIND_LIBRARY
#
# $1 = variable-prefix
# $2 = library
# $3 = subdirs
#
AC_DEFUN([METNO_FIND_LIBRARY], [
    if test x"${$1_LIBDIR}" = x ; then
	for DIR in METNO_FIND_DIRS $prefix ; do
	    for SUFFIX in lib \
	        m4_foreach([SUBDIR], [$3], [lib/SUBDIR SUBDIR/lib]) ; do
		# XXX check this list
		for LIB in lib$2.so lib$2.a $2.lib ; do
		    if test -r ${DIR}/${SUFFIX}/${LIB} ; then
			$1_LIBDIR=${DIR}/${SUFFIX}
			break 3
		    fi
		done
	    done
	done
    fi
])
