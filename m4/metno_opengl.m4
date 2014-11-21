# -*- autoconf -*-
#
# $Id: metno_opengl.m4 191 2010-01-20 12:00:34Z dages $
#
# SYNOPSIS
#
#   METNO_REQUIRE_OPENGL
#
# DESCRIPTION
#
#   The METNO_REQUIRE_OPENGL macro searches for the GL and GLU
#   libraries and terminates the configure script if they are not
#   present.
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
# METNO_REQUIRE_OPENGL
#
AC_DEFUN([METNO_REQUIRE_OPENGL], [
    # --with-opengl magic
    METNO_WITH_LIBRARY([OPENGL], [opengl], [OpenGL libraries], [yes])
    # save state
    AC_LANG_PUSH(C)
    saved_LIBS="${LIBS}"
    LIBS=""
    AC_CHECK_HEADER([GL/gl.h], [], [
	AC_MSG_ERROR([unable to locate GL header])
    ])
    METNO_SEARCH_LIBRARIES([OPENGL], [GL opengl32], [GL/gl.h],
	[glBegin], [],
	[], [AC_MSG_ERROR([GL library not found])])
    # XXX check result
    AC_CHECK_HEADER([GL/glu.h], [], [
	AC_MSG_ERROR([unable to locate GLU header])
    ])
    METNO_SEARCH_LIBRARIES([GL], [GLU glu32], [GL/gl.h GL/glu.h],
	[gluBeginCurve], [$OPENGL_LIBS],
	[], [AC_MSG_ERROR([GLU library not found])])
    # export our stuff
    AC_SUBST([OPENGL_LIBS])
    AC_SUBST([OPENGL_LDFLAGS])
    AC_SUBST([OPENGL_CPPFLAGS])
    # restore state
    LIBS="${saved_LIBS}"
    AC_LANG_POP(C)
])
