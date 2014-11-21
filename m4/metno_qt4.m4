# -*- autoconf -*-
#
# $Id: metno_qt4.m4 225 2012-08-27 11:10:59Z juergens $
#
# SYNOPSIS
#
#   METNO_REQUIRE_QT4(features)
#
# DESCRIPTION
#
#   The METNO_REQUIRE_QT4 macro searches for the Qt 4 library and
#   tools and terminates the configure script if they are not present.
#
# TODO
#
#   The METNO_REQUIRE_C_LIBRARY macro should be improved to the point
#   where this macro can simply use it instead of duplicating
#   significant parts of it.
#
#   De-duplicate the search for various binaries (moc, uic etc.)
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_REQUIRE_QT4
#
# $1 = list of features
#
# Qt 4 consists of a couple of tools, most importantly moc, and a
# large number of libraries, each with their own include directory.
# This was much simpler in Qt 3, which had one library and one include
# directory.
#
# Part of the problem is that some systems may have both Qt 3 and Qt 4
# installed; we need to figure out which is which.
#
AC_DEFUN([METNO_REQUIRE_QT4], [
    # --with-qt4 magic
    METNO_WITH_LIBRARY([QT4], [qt4], [Qt 4], [yes])
    AS_IF([test x"${QT4_DIR}" != x""],
	[QT4_PATH="${QT4_DIR}/bin${PATH_SEPARATOR}$PATH"],
	[QT4_PATH="${PATH}"])
    # save state
    AC_LANG_PUSH(C++)
    saved_CPPFLAGS="${CPPFLAGS}"
    saved_LDFLAGS="${LDFLAGS}"
    saved_LIBS="${LIBS}"
    LIBS=""

    # locate qmake; give the user a chance to specify where it is
    AC_ARG_WITH([qmake],
	AS_HELP_STRING([--with-qmake], [Qt build utility]), [
	QMAKE4=$withval
    ], [
	AC_PATH_PROGS([QMAKE4], [qmake-qt4 qmake4 qmake], [], [${QT4_PATH}])
    ])
    AS_IF([test x"${QMAKE4}" = x], [
	AC_MSG_ERROR([unable to locate qmake])
    ])
    # try to make sure it's Qt 4
    AS_IF([$QMAKE4 -v 2>&1 | grep -q "version 4\."], [], [
	AC_MSG_ERROR([incorrect rcc version])
    ])

    # locate moc; give the user a chance to specify where it is
    AC_ARG_WITH([moc],
	AS_HELP_STRING([--with-moc], [Qt meta-object compiler]), [
	MOC4=$withval
    ], [
	AC_PATH_PROGS([MOC4], [moc-qt4 moc4 moc], [], [${QT4_PATH}])
    ])
    AS_IF([test x"${MOC4}" = x], [
	AC_MSG_ERROR([unable to locate moc])
    ])

    # locate lupdate; give the user a chance to specify where it is
    AC_ARG_WITH([lupdate],
	AS_HELP_STRING([--with-lupdate], [Qt Linguist Update tool]), [
	LUPDATE4=$withval
    ], [
	AC_PATH_PROGS([LUPDATE4], [lupdate-qt4 lupdate4 lupdate], [], [${QT4_PATH}])
    ])
    AS_IF([test x"${LUPDATE4}" = x], [
	AC_MSG_ERROR([unable to locate lupdate])
    ])

    # locate lrelease; give the user a chance to specify where it is
    AC_ARG_WITH([lrelease],
	AS_HELP_STRING([--with-lrelease], [Qt Linguist Release tool]), [
	LRELEASE4=$withval
    ], [
	AC_PATH_PROGS([LRELEASE4], [lrelease-qt4 lrelease4 lrelease], [], [${QT4_PATH}])
    ])
    AS_IF([test x"${LRELEASE4}" = x], [
	AC_MSG_ERROR([unable to locate lrelease])
    ])

    # locate uic; give the user a chance to specify where it is
    AC_ARG_WITH([uic],
	AS_HELP_STRING([--with-uic], [Qt user interface compiler]), [
	UIC4=$withval
    ], [
	AC_PATH_PROGS([UIC4], [uic-qt4 uic4 uic], [], [${QT4_PATH}])
    ])
    AS_IF([test x"${UIC4}" = x], [
	AC_MSG_ERROR([unable to locate uic])
    ])

    # locate rcc; give the user a chance to specify where it is
    AC_ARG_WITH([rcc],
	AS_HELP_STRING([--with-rcc], [Qt resource compiler]), [
	RCC4=$withval
    ], [
	AC_PATH_PROGS([RCC4], [rcc-qt4 rcc4 rcc], [], [${QT4_PATH}])
    ])
    AS_IF([test x"${RCC4}" = x], [
	AC_MSG_ERROR([unable to locate rcc])
    ])

    # Use qmake to determine compiler and linker flags; inspired by
    # but not derived from AutoTroll by Benoit Sigoure.
    #
    # First, we create a) a header that includes the headers for all
    # the Qt components we are interested in and declares a subclass
    # of QObject, b) a do-nothing program that includes our header,
    # and c) a qmake project file.
    #
    # Next, we run qmake to generate a makefile.  We then extract all
    # those juicy preprocessor, compiler and linker flags by adding a
    # target to the makefile that prints them, running it, and rooting
    # through the output.
    #
    # Finally, we run moc on our header and try to compile and link
    # the result.

    # Grrr, this is something autoconf should do for us.
    qtmpdir=`mktemp -d 2>/dev/null`
    if test -z "${qtmpdir}" ; then
	qtmpdir="metno_qt4.${RANDOM}"
dnl	if test -n "${TMPDIR}" -a -d "${TMPDIR}" ; then
dnl	    qtmpdir="${TMPDIR}/${qtmpdir}"
dnl	elif test -n "${TEMP}" -a -d "${TEMP}" ; then
dnl	    qtmpdir="${TEMP}/${qtmpdir}"
dnl	elif test -n "${TMP}" -a -d "${TMP}" ; then
dnl	    qtmpdir="${TMP}/${qtmpdir}"
dnl	elif test -d /tmp ; then
dnl	    qtmpdir="/tmp/${qtmpdir}"
dnl	else
	    # last resort, probably won't work due to qmake's
	    # insistance on generating relative paths.
	    qtmpdir="`(cd ${ac_aux_dir} && pwd)`/${qtmpdir}"
dnl	fi
	if ! mkdir "${qtmpdir}" 2>/dev/null ; then
	    AC_MSG_ERROR([failed to create temporary directory])
	fi
    fi
    qheader="metno_qt4.h"
    qsource="metno_qt4.cc"
    qpro="metno_qt4.pro"
    qmakefile="metno_qt4.makefile"
    # header
    cat >"${qtmpdir}/${qheader}" <<METNO_QT4_EOF
@%:@include <QObject>
m4_foreach_w([feature], $1, [
@%:@include <m4_if(feature, [Qt3Support], [Qt3Support], [Qt[]feature])>
])
class metno_qt4: public QObject { Q_OBJECT };
METNO_QT4_EOF
    # program
    cat >"${qtmpdir}/${qsource}" <<METNO_QT4_EOF
@%:@include "${qheader}"
int main(int argc, char **argv) { (void)argc; (void)argv; return 0; }
METNO_QT4_EOF
    # project file
    cat >"${qtmpdir}/${qpro}" <<METNO_QT4_EOF
TEMPLATE = app
CONFIG += release static
TARGET = metno_qt4
QT = m4_translit(m4_normalize([$1]), [A-Z], [a-z])
SOURCES = ${qsource}
METNO_QT4_EOF
    # run qmake
    ${QMAKE4} -makefile -o "${qtmpdir}/${qmakefile}" "${qtmpdir}/${qpro}"
    if test -f "${qtmpdir}/${qmakefile}.Release" ; then
	qmakefile="${qmakefile}.Release"
    fi
    AS_IF([test -r "${qtmpdir}/${qmakefile}"], [], [
	rm -rf "${qtmpdir}"
	AC_MSG_ERROR([failed to run qmake])
    ])
    # extract flags
    cat >>"${qtmpdir}/${qmakefile}" <<METNO_QT4_EOF
metno-qt4-flags:
	@echo \${DEFINES} \${INCPATH} \${LIBS} \${LFLAGS}
METNO_QT4_EOF
    for arg in `${MAKE-make} -C "${qtmpdir}" -f "${qmakefile}" metno-qt4-flags` ; do
	AS_CASE([$arg],
	    [-D*], [QT4_CPPFLAGS="${QT4_CPPFLAGS} $arg"],
	    [-I.|-Irelease|-Idebug], [], # superfluous - ignore
	    [-I*], [QT4_CPPFLAGS="${QT4_CPPFLAGS} $arg"],
	    [-L*], [QT4_LIBS="${QT4_LIBS} $arg"],
	    [-l*], [QT4_LIBS="${QT4_LIBS} $arg"],
dnl	    [-Wl,*], [QT4_LDFLAGS="${QT4_LDFLAGS} $arg"],
	    [])
    done
    AS_IF([test x"${target_os}" = x"mingw32"], [
	QT4_DLL=mingwm10.dll
	for lib in ${QT4_LIBS} ; do
	    AS_CASE([$lib],
		[-lQt*], [QT4_DLL="${QT4_DLL} ${lib@%:@-l}.dll"],
		[])
	done])
    # test moc and compile / link test program
    AC_MSG_CHECKING([whether moc and the Qt libraries work])
    LIBS="${QT4_LIBS}"
    LDFLAGS="${QT4_LDFLAGS}"
    CPPFLAGS="${QT4_CPPFLAGS}"
    AC_LINK_IFELSE([AC_LANG_SOURCE([`${MOC4} "${qtmpdir}/${qheader}"`
	int main(int argc, char **argv) { (void)argc; (void)argv; return 0; }
    ])], [
	AC_MSG_RESULT([yes])
    ], [
	AC_MSG_RESULT([no])
	rm -rf "${qtmpdir}"
	AC_MSG_ERROR([failed to compile test program])
    ])
    rm -rf "${qtmpdir}"
    AC_SUBST([QT4_DLL])
    AC_SUBST([QT4_LIBS])
    AC_SUBST([QT4_LDFLAGS])
    AC_SUBST([QT4_CPPFLAGS])
    # restore state
    LIBS="${saved_LIBS}"
    LDFLAGS="${saved_LDFLAGS}"
    CPPFLAGS="${saved_CPPFLAGS}"
    AC_LANG_POP(C++)
])
