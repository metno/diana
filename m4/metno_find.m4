# -*- autoconf -*-
#
# $Id: metno_find.m4 149 2009-12-07 13:01:42Z dages $
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
# $2 = libraries
# $3 = subdirs
#
AC_DEFUN([METNO_FIND_LIBRARY], [
    if test x"${$1_LIBDIR}" != x ; then
	FIND_DIRS="${$1_LIBDIR}"
    else
	for DIR in METNO_FIND_DIRS $prefix ; do
	    for SUFFIX in lib \
		m4_foreach([SUBDIR], [$3], [lib/SUBDIR SUBDIR/lib]) ; do
		FIND_DIRS="${FIND_DIRS} ${DIR}/${SUFFIX}"
	    done
	done
    fi
    for DIR in ${FIND_DIRS} ; do
	for LIBNAME in $2 ; do
	    for LIB in lib$LIBNAME.so lib$LIBNAME.dll.a lib$LIBNAME.a ; do
		if test -r ${DIR}/${LIB} ; then
		    $1_LIBNAME=${LIBNAME}
		    $1_LIBDIR=${DIR}
		    break 3
		fi
	    done
	done
    done
])
