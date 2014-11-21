# -*- autoconf -*-
#
# SYNOPSIS
#
#   METNO_SET_VERSION_INFO [(VERSION [,PREFIX])]
#
# DESCRIPTION
#
#   Defaults:
#
#     $1 = $PACKAGE_VERSION
#     $2 = <none>
#
#   Split the $VERSION number into libtool's $CURRENT, $REVISION, and
#   $AGE version fields.  These fields are combined to form
#   @VERSION_INFO@.
#
#   Example: A VERSION="3.8.2" will be transformed into
#
#      VERSION_INFO = -version-info 3:8:2
#
#   then push that variable to your libtool linker
#
#      libtest_la_LIBADD = @VERSION_INFO@
#
#   For a linux-target this will tell libtool to install the lib as
#
#      libmy.so libmy.la libmy.a libmy.so.3 libmy.so.3.8.2
#
#   Guidelines for determinig the version info:
#
#     1. If you have changed any of the sources for this library, the
#        revision number must be incremented. This is a new revision
#        of the current interface.
#     
#     2. If the interface has changed, then current must be
#        incremented, and revision reset to `0'. This is the first
#        revision of a new interface.
#     
#     3. If the new interface is a superset of the previous interface
#        (that is, if the previous interface has not been broken by
#        the changes in this new release), then age must be
#        incremented. This release is backwards compatible with the
#        previous release.
#     
#     4. If the new interface has removed elements with respect to the
#        previous interface, then you have broken backward
#        compatibility and age must be reset to `0'. This release has
#        a new, but backwards incompatible interface.
# 
#     (Source: http://sourceware.org/autobook/autobook/autobook_91.html)
#
# AUTHORS
#
#   Martin Thorsen Ranang <mtr@linpro.no>
#
# THANKS
#
#   This file was based on ax_set_version_info.m4 by Guido U. Draheim
#   <guidod@gmx.de>.
#
# LAST MODIFICATION
#
#   $Date: 2007-12-03 09:22:35 +0100 (Mon, 03 Dec 2007) $
#
# ID
#
#   $Id: metno_set_version_info.m4 149 2009-12-07 13:01:42Z dages $
#
# COPYLEFT
#
#   Copyright (c) 2006 Meterologisk Institutt <diana@met.no>
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2 of the
#   License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#   02111-1307, USA.
#
#   As a special exception, the respective Autoconf Macro's copyright
#   owner gives unlimited permission to copy, distribute and modify the
#   configure scripts that are the output of Autoconf when processing
#   the Macro. You need not follow the terms of the GNU General Public
#   License when using or distributing such scripts, even though
#   portions of the text of the Macro appear in them. The GNU General
#   Public License (GPL) does govern all other use of the material that
#   constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the
#   Autoconf Macro released by the Autoconf Macro Archive. When you
#   make and distribute a modified version of the Autoconf Macro, you
#   may extend this special exception to the GPL to apply to your
#   modified version as well.
#
AC_DEFUN([METNO_SET_VERSION_INFO],[dnl
AS_VAR_PUSHDEF([MAJOR],ifelse($2,,[CURRENT],[$2_CURRENT]))dnl
AS_VAR_PUSHDEF([MINOR],ifelse($2,,[REVISION],[$2_REVISION]))dnl
AS_VAR_PUSHDEF([MICRO],ifelse($2,,[AGE],[$2_AGE]))dnl
AS_VAR_PUSHDEF([PATCH],ifelse($2,,[PATCH_VERSION],[$2_PATCH_VERSION]))dnl
AS_VAR_PUSHDEF([LTVER],ifelse($2,,[VERSION_INFO],[$2_VERSION_INFO]))dnl
test ".$PACKAGE_VERSION" = "." && PACKAGE_VERSION="$VERSION"
AC_MSG_CHECKING(ifelse($2,,,[$2 ])out linker version info dnl
ifelse($1,,$PACKAGE_VERSION,$1) )
  MINOR=`echo ifelse( $1, , $PACKAGE_VERSION, $1 )`
  MAJOR=`echo "$MINOR" | sed -e 's/[[.]].*//'`
  MINOR=`echo "$MINOR" | sed -e "s/^$MAJOR//" -e 's/^.//'`
  MICRO="$MINOR"
  MINOR=`echo "$MICRO" | sed -e 's/[[.]].*//'`
  MICRO=`echo "$MICRO" | sed -e "s/^$MINOR//" -e 's/^.//'`
  PATCH="$MICRO"
  MICRO=`echo "$PATCH" | sed -e 's/[[^0-9]].*//'`
  PATCH=`echo "$PATCH" | sed -e "s/^$MICRO//" -e 's/[[-.]]//'`
  if test "_$MICRO" = "_" ; then MICRO="0" ; fi
  if test "_$MINOR" = "_" ; then MINOR="$MAJOR" ; MAJOR="0" ; fi
  MINOR=`echo "$MINOR" | sed -e 's/[[^0-9]].*//'`
  LTVER="-version-info $MAJOR:$MINOR:$MICRO"
AC_MSG_RESULT([$MAJOR:$MINOR:$MICRO (*.so.$MAJOR.$MINOR.$MICRO)])
AC_SUBST(LTVER)
AS_VAR_POPDEF([LTVER])dnl
AS_VAR_POPDEF([PATCH])dnl
AS_VAR_POPDEF([MICRO])dnl
AS_VAR_POPDEF([MINOR])dnl
AS_VAR_POPDEF([MAJOR])dnl
])
