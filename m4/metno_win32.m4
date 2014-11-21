# -*- autoconf -*-
#
# $Id: metno_win32.m4 209 2010-03-18 16:43:28Z dages $
#
# SYNOPSIS
#
#   METNO_WIN32(version)
#   METNO_WIN32_IFELSE(action-if-true, action-if-false)
#   METNO_WIN32_NO_GDI
#   METNO_WIN32_NO_UNICODE
#   METNO_WIN32_WINSOCK
#
# DESCRIPTION
#
#   The METNO_WIN32 macro defines the win32 shell variable to "yes" if
#   the target system runs some variant of Win32 and "no" otherwise.
#   It also defines a corresponding automake conditional, WIN32, and
#   four substitution variables: WIN32_VERSION, WIN32_CPPFLAGS,
#   WIN32_LDFLAGS and WIN32_LIBS.  WIN32_VERSION indicates the desired
#   API compatibility level, and WIN32_CPPFLAGS is initialized to
#   reflect this.  If a version is specified, WIN32_VERSION is set to
#   that version; otherwise, it is set to 0x0501.  Useful values are:
#
#     0x0500    Windows 2000
#     0x0501    Windows XP / Windows Server 2003
#     0x0502    Windows XP SP2 / Windows Server 2003 SP1
#     0x0600    Windows Vista / Windows Server 2008
#     0x0601    Windows 7
#
#   The METNO_WIN32_IFELSE macro runs action-if-true if the target
#   system runs some variant of Win32 and action-if-false otherwise.
#
#   The METNO_WIN32_NO_GDI macro adds -DNOGDI to WIN32_CPPFLAGS to
#   avoid namespace conflicts with the Win32 graphics API.  Use only
#   when you do not intend to use GDI.
#
#   The METNO_WIN32_NO_UNICODE macro adds -UUNICODE to WIN32_CPPFLAGS.
#   When both single-byte and multi-byte versions of an API call exist
#   (e.g. GetUserNameA() / GetUserNameW()), the unqualified name
#   (GetUserName()) will default to the single-byte version.
#
#   The METNO_WIN32_WINSOCK will add -lws2_32 to WIN32_LIBS.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_WIN32_IFELSE
#
# $1 = action-if-true
# $2 = action-if-false
#
AC_DEFUN([METNO_WIN32_IFELSE], [
    AS_IF([test x"${win32}" = x"yes"], [:;$1], [:;$2])
])

#
# METNO_WIN32
#
# $1 = version
#
AC_DEFUN_ONCE([METNO_WIN32], [
    AS_CASE([$target_os],
    [mingw32], [
	win32=yes
    ], [
	win32=no
    ])
    AM_CONDITIONAL([WIN32], [test x"${win32}" = x"yes"])
    METNO_WIN32_IFELSE([
	WIN32_VERSION=m4_if([$1], [], [0x0501], ["$1"])
	WIN32_CPPFLAGS="${WIN32_CPPFLAGS} -DWINVER=${WIN32_VERSION}"
    ], [
	WIN32_VERSION=0x0000
    ])
    AC_SUBST([WIN32_VERSION])
    AC_SUBST([WIN32_CPPFLAGS])
    AC_SUBST([WIN32_LDFLAGS])
    AC_SUBST([WIN32_LIBS])
])

#
# METNO_WIN32_NO_GDI
#
AC_DEFUN_ONCE([METNO_WIN32_NO_GDI], [
    AC_REQUIRE([METNO_WIN32])
    METNO_WIN32_IFELSE([WIN32_CPPFLAGS="${WIN32_CPPFLAGS} -DNOGDI"])
])

#
# METNO_WIN32_NO_UNICODE
#
AC_DEFUN_ONCE([METNO_WIN32_NO_UNICODE], [
    AC_REQUIRE([METNO_WIN32])
    METNO_WIN32_IFELSE([WIN32_CPPFLAGS="${WIN32_CPPFLAGS} -UUNICODE"])
])

#
# METNO_WIN32_WINSOCK
#
AC_DEFUN_ONCE([METNO_WIN32_WINSOCK], [
    AC_REQUIRE([METNO_WIN32])
    METNO_WIN32_IFELSE([WIN32_LIBS="${WIN32_LIBS} -lws2_32"])
])
