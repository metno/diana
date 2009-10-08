# -*- autoconf -*-
#
# $Id: metno_opengl.m4 2766 2009-09-14 04:44:08Z dages $
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
    METNO_WITH_LIBRARY([OpenGL])
    # save state
    AC_LANG_PUSH(C)
    saved_LIBS="${LIBS}"
    LIBS=""
    AC_CHECK_HEADER([GL/gl.h], [], [
	AC_MSG_ERROR([unable to locate GL header])
    ])
    AC_CHECK_LIB([GL], [main], [], [
	AC_MSG_ERROR([libGL not found])
    ])
    AC_CHECK_HEADER([GL/glu.h], [], [
	AC_MSG_ERROR([unable to locate GLU header])
    ])
    AC_CHECK_LIB([GLU], [main], [], [
	AC_MSG_ERROR([libGLU not found])
    ])
    # export our stuff
    OPENGL_LIBS="${LIBS}"
    AC_SUBST([OPENGL_LIBS])
    AC_SUBST([OPENGL_LDFLAGS])
    AC_SUBST([OPENGL_CPPFLAGS])
    # restore state
    LIBS="${saved_LIBS}"
    AC_LANG_POP(C)
])
