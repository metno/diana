# -*- autoconf -*-
#
# $Id: metno_qt4.m4 2766 2009-09-14 04:44:08Z dages $
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
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_REQUIRE_Q4
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
    METNO_WITH_LIBRARY([qt4])
    # save state
    AC_LANG_PUSH(C++)
    saved_CPPFLAGS="${CPPFLAGS}"
    saved_LDFLAGS="${LDFLAGS}"
    saved_LIBS="${LIBS}"
    LIBS=""
    # locate moc; give the user a chance to specify where it is
    AC_ARG_WITH([moc],
	AS_HELP_STRING([--with-moc], [Qt meta-object compiler]), [
	MOC4=$withval
    ], [
	AC_CHECK_PROGS([MOC4], [moc-qt4 moc4 moc])
    ])
    AS_IF([test x"${MOC4}" = x], [
	AC_MSG_ERROR([unable to locate moc])
    ])
    # try to make sure it's Qt 4
    AS_IF([$MOC4 -v 2>&1 | grep -q "Qt 4"], [], [
	AC_MSG_ERROR([incorrect moc version])
    ])
    # look for headers and libraries
    # Qt 4 makes this perversely difficult.
    METNO_FIND_HEADER([QT4], [Qt/QtCore], [qt4])
    AS_IF([test x"${QT4_INCLUDEDIR}" != x], [
	CPPFLAGS="${CPPFLAGS} -I${QT4_INCLUDEDIR}"
    ], [
	AC_MSG_ERROR([unable to locate Qt 4 include directory])
    ])
    # look for Qt3Support
    m4_foreach_w([FEATURE], $1, [
	m4_if(FEATURE, [Qt3Support], [
	    DQT3SUPPORT="-DQT3_SUPPORT"
	    CPPFLAGS="${CPPFLAGS} ${DQT3SUPPORT}"
	])
    ])
    m4_foreach_w([feature], $1, [
	m4_pushdef([FEATURE], m4_translit(feature, [a-z/.-], [A-Z___]))
	FEATURE[]_CPPFLAGS="-I${QT4_INCLUDEDIR}/feature ${DQT3SUPPORT}"
	FEATURE[]_LIBS="-l[]feature"
	CPPFLAGS="${CPPFLAGS} -I${QT4_INCLUDEDIR}/feature"
	m4_popdef([FEATURE])
	AC_CHECK_HEADER(feature, [], [
	    AC_MSG_ERROR([unable to locate Qt 4 component: feature])
	], [#include <Qt/qglobal.h>])
	AC_CHECK_LIB(feature, [main], [], [
	    AC_MSG_ERROR([required Qt 4 component feature not found])
	])
    ])
    # test moc and the library
    # XXX fairly ugly
    AC_MSG_CHECKING([whether moc and the Qt libraries work])
    saved_ac_ext=$ac_ext
    ac_ext=h
    AC_LANG_CONFTEST([#include <QObject>
	class conftest: public QObject { Q_OBJECT };
    ])
    ac_ext=$saved_ac_ext
    AC_LINK_IFELSE([`$MOC4 conftest.h`
	int main(void) { return 0; }
    ], [
	AC_MSG_RESULT([yes])
    ], [
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([failed to compile test program])
    ])
    # export our stuff
    QT4_LIBS="${LIBS}"
    QT4_CPPFLAGS="${CPPFLAGS}"
    AC_SUBST([QT4_LIBS])
    AC_SUBST([QT4_CPPFLAGS])
    # restore state
    LIBS="${saved_LIBS}"
    LDFLAGS="${saved_LDFLAGS}"
    CPPFLAGS="${saved_CPPFLAGS}"
    AC_LANG_POP(C++)
])
