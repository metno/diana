# -*- autoconf -*-
#
# $Id: metno_with.m4 181 2010-01-18 18:11:34Z dages $
#
# SYNOPSIS
#
#   METNO_WITH_LIBRARY(variable-prefix, option-name, pretty-name, default)
#   METNO_WITH(variable-prefix, option-name, pretty-name, default)
#   METNO_IF_WITH(option-name, action-if-with, action-if-without)
#   METNO_IF_WITH_ANY(option-names, action-if-with, action-if-without)
#   METNO_IF_WITH_ALL(option-names, action-if-with, action-if-without)
#   METNO_WITH_CONDITIONAL(variable-prefix, option-name)
#
# DESCRIPTION
#
#   The METNO_WITH_LIBRARY macro creates three command-line options
#   allowing the user to enable or disable, and specify the location
#   of, an external library.  The options are:
#
#       --with-<option-name>
#       --with-<option-name>-libdir
#       --with-<option-name>-includedir
#
#   If the --with-<option-name> command-line option is specified and
#   its value is not "no", the with_<option-name> shell variable is
#   set to "yes".  If the option's value is not "yes", the
#   <variable-prefix>_DIR shell variable is set to <value>, the
#   <variable-prefix>_LIBDIR shell variable is set to <value>/lib, and
#   the <variable-prefix>_INCLUDEDIR variable is set to
#   <value>/include.  The latter two can be overridden by the
#   --with-<option-name>-libdir and --with-<option-name>-includedir
#   command line options, respectively.  If the option is not
#   specified, its value is set to <default>, and the remaining
#   variables are set accordingly.
#
#   Finally, <variable-prefix>_DIR, <variable-prefix>_LIBDIR and
#   <variable-prefix>_INCLUDEDIR are AC_SUBST'ed.
#
#   The <pretty-name> parameter is used in the help strings for the
#   three command-line options.
#
#   The macro makes no attempt to verify the library's presence or
#   usability.
#
#   The METNO_WITH macro is a simplified version which creates only
#   the --with-<option-name> command-line option, and defines only the
#   with_<option-name> shell variable.
#
#   The METNO_IF_WITH macro executes <action-if-with> if the value of
#   the with_<option-name> shell variable is "yes", and
#   <action-if-without> otherwise.  Either or both actions can be
#   empty.
#
#   The METNO_IF_WITH_ANY macro executes <action-if-with> if the
#   with_<option> shell variable is "yes" for any of the specified
#   options, and <action-if-without> otherwise.  Either or both
#   actions can be empty.
#
#   The METNO_IF_WITH_ALL macro executes <action-if-with> if the
#   with_<option> shell variable is "yes" for all of the specified
#   options, and <action-if-without> otherwise.  Either or both
#   actions can be empty.
#
#   The METNO_WITH_CONDITIONAL macro defines an automake conditional
#   named WITH_<variable-prefix> which is true if and only if the
#   value of the with_<option-name> shell variable is "yes".  This is
#   necessary because automake conditionals can not be defined within
#   conditional code.
#
# NOTES
#
#   The ":;" in the AS_IFs in the predicate macros are there in case
#   the respective action is empty; some shells croak if there is
#   nothing between "then" and "else" or between "else" and "fi".
#
# TODO
#
#   Better help strings.
#
#   Better handling of default case (see metno_enable.m4)
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_WITH_LIBRARY
#
# $1 = variable-prefix
# $2 = option-name
# $3 = pretty-name
# $4 = default
#
AC_DEFUN([METNO_WITH_LIBRARY], [
    # --with-<option-name>
    AC_ARG_WITH([$2],
	AS_HELP_STRING([--with-$2=DIR], [$3]), [
	AS_CASE([${withval}],
	[yes], [
	    AS_TR_SH([with_$2])=yes
	],
	[no], [
	    AS_TR_SH([with_$2])=no
	], [
	    AS_TR_SH([with_$2])=yes
	    $1_DIR=${withval}
	    $1_LIBDIR=${$1_DIR}/lib
	    $1_INCLUDEDIR=${$1_DIR}/include
	])
    ], [
	AS_TR_SH([with_$2])=m4_if([$4], [], [no], [$4])
    ])
    # --with-<option-name>-libdir
    AC_ARG_WITH([$2-libdir],
	AS_HELP_STRING([--with-$2-libdir=DIR], [$3 library directory]),
	[$1_LIBDIR=$withval])
    # --with-<option-name>-includedir
    AC_ARG_WITH([$2-includedir],
	AS_HELP_STRING([--with-$2-includedir=DIR], [$3 include directory]),
	[$1_INCLUDEDIR=$withval])
    # export our stuff
    AC_SUBST([$1_DIR])
    AC_SUBST([$1_LIBDIR])
    AC_SUBST([$1_INCLUDEDIR])
])

#
# METNO_WITH
#
# $1 = variable-prefix
# $2 = option-name
# $3 = pretty-name
# $4 = default
#
AC_DEFUN([METNO_WITH], [
    # --with-<option-name>
    AC_ARG_WITH([$2],
	AS_HELP_STRING([--with-$2], [$3]), [
	AS_CASE([${withval}],
	[yes], [
	    with_$2=yes
	],
	[no], [
	    with_$2=no
	], [
	    AC_MSG_ERROR([bad value for --with-$2])
	])
    ], [
	with_$2=$4
    ])
])

#
# METNO_IF_WITH
#
# $1 = option-name
# $2 = action-if-with
# $3 = action-if-disabled
#
AC_DEFUN([METNO_IF_WITH], [
    AS_IF([test x"$with_$1" = x"yes"], [:;$2], [:;$3])
])

#
# METNO_IF_WITH_ANY
#
# $1 = option-names
# $2 = action-if-with
# $3 = action-if-without
#
AC_DEFUN([METNO_IF_WITH_ANY], [
    metno_if_with_any_result=no
    m4_foreach_w(COMPONENT, [$1], [
	AS_IF([test x"$with_[]COMPONENT" = x"yes"],
	    [metno_if_with_any_result=yes; break])
    ])
    AS_IF([test x"$metno_if_with_any_result" = x"yes"], [:;$2], [:;$3])
    unset metno_if_with_any_result
])

#
# METNO_IF_WITH_ALL
#
# $1 = option-names
# $2 = action-if-with
# $3 = action-if-without
#
AC_DEFUN([METNO_IF_WITH_ALL], [
    metno_if_with_all_result=yes
    m4_foreach_w(COMPONENT, [$1], [
	AS_IF([test x"$with_[]COMPONENT" != x"yes"],
	    [metno_if_with_all_result=no; break])
    ])
    AS_IF([test x"$metno_if_with_all_result" = x"yes"], [:;$2;], [:;$3])
    unset metno_if_with_all_result
])

#
# METNO_WITH_CONDITIONAL
#
# $1 = variable-prefix
# $2 = option-name
#
AC_DEFUN([METNO_WITH_CONDITIONAL], [
    AM_CONDITIONAL([WITH_$1], [test x"$with_$2" = x"yes"])
])
