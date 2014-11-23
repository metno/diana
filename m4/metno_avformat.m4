# -*- autoconf -*-
#
# $Id: metno_avformat.m4 215 2010-09-09 13:37:52Z lisbethb $
#
# SYNOPSIS
#
#   METNO_REQUIRE_AVFORMAT(default)
#
# DESCRIPTION
#
#   The METNO_WITH_AVFORMAT macro searches for the FFMPEG libraries
#   (libavformat etc.)
#
#   The METNO_REQUIRE_AVFORMAT macro is equivalent, but terminates the
#   configure script if it can not find the Avformat libraries.
#
#   NOTE: we can't use pkg-config for this, because ffmpeg's .pc files
#   lack dependency information.
#
# TODO
#
#   Do not assume that libz is present and reachable with the default
#   include and library paths.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_WITH_AVFORMAT
#
# $1 = default
#
AC_DEFUN([METNO_WITH_AVFORMAT], [
    # --with-avformat magic
    METNO_WITH_LIBRARY([avformat], [avformat], [avformat library], [$1])

    # is libavformat required, or did the user request it?
    AS_IF([test x"${with_avformat}" != x"no" -o x"${require_avformat}" = x"yes"], [
	# save state
	saved_CPPFLAGS="${CPPFLAGS}"
	saved_LDFLAGS="${LDFLAGS}"
	saved_LIBS="${LIBS}"
	LIBS=""

	# header location
	#
	# A stock installation (from source) installs the libavformat
	# headers in ${prefix}/include/libavformat, the libavcodec
	# headers in ${prefix}/include/libavcodec, etc.	 The Ubuntu
	# packages, however, install them in ${prefix}/include/ffmpeg.
	# The instructions for building Diana on Windows on the Diana
	# wiki (diana.wiki.met.no) explain how to reproduce this.
	#
	METNO_FIND_HEADER([AVFORMAT], [avformat.h], [libavformat])
	METNO_FIND_HEADER([AVFORMAT], [avformat.h], [ffmpeg])
	AS_IF([test x"${AVFORMAT_INCLUDEDIR}" != x""], [
	    AVFORMAT_CPPFLAGS="-I${AVFORMAT_INCLUDEDIR}"
	])
	CPPFLAGS="${CPPFLAGS} ${AVFORMAT_CPPFLAGS}"

	# library location
	AS_IF([test x"${AVFORMAT_LIBDIR}" != x""], [
	    AVFORMAT_LDFLAGS="-L${AVFORMAT_LIBDIR}"
	])
	LDFLAGS="${LDFLAGS} ${AVFORMAT_LDFLAGS}"

	AC_LANG_PUSH(C)
	AC_CHECK_HEADER([avformat.h], [], [
	    AC_MSG_ERROR([the required avformat.h header was not found])
	])
	AC_CHECK_LIB([avutil], [av_crc], [], [
	    AC_MSG_ERROR([the required avutil library was not found])
	])
	# lz hack: avcodec needs -lz, but autoconf will insert it in
	# lz hack: the wrong position if we list it in other-libs
	LIBS="${LIBS} -lz"
	AC_CHECK_LIB([avcodec], [av_find_opt], [], [
	    AC_MSG_ERROR([the required avcodec library was not found])
	])
	AC_CHECK_LIB([avformat], [av_read_frame], [], [
	    AC_MSG_ERROR([the required avformat library was not found])
	])
	# lz hack: undo
	LIBS="${LIBS% -lz}"
	AC_LANG_POP(C)

	# export our stuff
	AVFORMAT_LIBS="${LIBS}"
	AC_SUBST([AVFORMAT_LIBS])
	AC_SUBST([AVFORMAT_LDFLAGS])
	AC_SUBST([AVFORMAT_CPPFLAGS])

	# restore state
	LIBS="${saved_LIBS}"
	LDFLAGS="${saved_LDFLAGS}"
	CPPFLAGS="${saved_CPPFLAGS}"
    ])
])

#
# METNO_REQUIRE_AVFORMAT
#
AC_DEFUN([METNO_REQUIRE_AVFORMAT], [
    require_avformat=yes
    METNO_WITH_AVFORMAT
])
