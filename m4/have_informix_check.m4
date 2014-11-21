# autoconf macros for detecting Informix API
#
# 20080214 MLS

AC_DEFUN([HAVE_INFORMIX_CHECK],[
AC_ARG_WITH(Informix,
      [  --with-Informix=DIR      prefix for Informix files],
            [case "$withval" in
               yes) INFORMIXDIR=/opt/informix ;;
               no)  INFORMIXDIR= ;;
               *)   INFORMIXDIR=${enableval} ;;
             esac], [INFORMIXDIR=/opt/informix]
             ,
      [INFORMIXDIR=$INF_DIR])

AC_ARG_WITH(Informix-libdir,
      [  --with-Informix-libdir=DIR      prefix for Informix library files],
            [if test "$withval" = "no"; then
               INF_LIBS_DIR=
             else
               INF_LIBS_DIR="$withval"
             fi],
      [INF_LIBS_DIR=$INFORMIXDIR/lib])

AC_ARG_WITH(Informix-includedir,
      [  --with-Informix-includedir=DIR      prefix for Informix headers],
            [if test "$withval" = "no"; then
               INF_INCLUDES_DIR=
             else
               INF_INCLUDES_DIR="$withval"
             fi],
      [INF_INCLUDES_DIR=$INFORMIXDIR/incl])

AC_CHECKING([for Informix])

AC_MSG_RESULT([Informix lib location: $INF_LIBS_DIR])
AC_MSG_RESULT([Informix headers location: $INF_INCLUDES_DIR])

if eval "test ! -d $INF_INCLUDES_DIR"; then
    AC_MSG_ERROR([$INF_INCLUDES_DIR not found])
fi

if eval "test -d $INF_LIBS_DIR"; then
    INF_LDFLAGS="-L$INF_LIBS_DIR -L$INF_LIBS_DIR/esql -L$INF_LIBS_DIR/dmi -pthread"
    INF_LIBS="-lthsql -lthasf -lthgen -lthos -lifgls -lifglx -lthdmi -lm -ldl -lcrypt -lnsl $INF_LIBS_DIR/esql/checkapi.o"
else
    AC_MSG_ERROR([$INF_LIBS_DIR not found])
fi

save_CPPFLAGS="$CPPFLAGS"
save_LDFLAGS="$LDFLAGS"
save_LIBS="$LIBS"

CPPFLAGS="$CPPFLAGS -I$INF_INCLUDES_DIR -I$INF_INCLUDES_DIR/esql -I$INF_INCLUDES_DIR/dmi"
LIBS="$LIBS $INF_LIBS"
LDFLAGS="$LDFLAGS $INF_LDFLAGS"

informix=no

AC_CHECKING([for milib.h])
AC_CHECK_HEADER([milib.h],
                [informix=yes], [informix=no])

AC_MSG_RESULT([Informix include result: $informix])
if test "x$informix" = xno; then
    AC_MSG_ERROR([Informix include not found])
fi

AC_CHECKING([for libthdmi.so])
AC_CHECK_LIB([thdmi], [main],
           [informix=yes], [informix=no])

AC_MSG_RESULT([Informix link result: $informix])
if test "x$informix" = xno; then
    AC_MSG_ERROR([Informix library not found])
fi

CPPFLAGS="$save_CPPFLAGS"
LDFLAGS="$save_LDFLAGS"
LIBS="$save_LIBS"

AC_SUBST(INFORMIX_CPPFLAGS, ["-I$INF_INCLUDES_DIR -I$INF_INCLUDES_DIR/esql -I$INF_INCLUDES_DIR/dmi"])
AC_SUBST(INFORMIX_LDFLAGS, [$INF_LDFLAGS])
AC_SUBST(INFORMIX_LIBS, [$INF_LIBS])

])
