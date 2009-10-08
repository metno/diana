# -*- autoconf -*-
#
# $Id: metno_enable.m4 2787 2009-09-23 17:26:07Z dages $
#
# SYNOPSIS
#
#   METNO_ENABLE(variable-prefix, option-name, pretty-name, default)
#   METNO_IF_ENABLED(option-name, action-if-enabled, action-if-disabled)
#   METNO_IF_ENABLED_ANY(opbtion-names, action-if-enabled, action-if-disabled)
#   METNO_IF_ENABLED_ALL(option-names, action-if-enabled, action-if-disabled)
#   METNO_ENABLE_CONDITIONAL(variable-prefix, option-name)
#
# DESCRIPTION
#
#   The METNO_ENABLE macro creates a --enable-<option-name>
#   command-line option allowing the user to enable or disable a
#   specific feature.
#
#   If either --enable-<option-name> or --disable-<option-name> is
#   specified, the with_<option-name> shell variable is set according
#   to the user's wishes.  Otherwise, it is set to <default>.
#
#   The <pretty-name> parameter is used in the help string for the
#   command-line option.
#
#   The METNO_IF_ENABLED macro executes <action-if-enabled> if the
#   value of the enabled_<option-name> shell variable is "yes", and
#   <action-if-disabled> otherwise.  Either or both actions can be
#   empty.
#
#   The METNO_IF_ENABLED_ANY macro executes <action-if-enabled> if the
#   enabled_<option> shell variable is "yes" for any of the specified
#   options, and <action-if-disabled> otherwise.  Either or both
#   actions can be empty.
#
#   The METNO_IF_ENABLED_ALL macro executes <action-if-enabled> if the
#   enabled_<option> shell variable is "yes" for all of the specified
#   options, and <action-if-disabled> otherwise.  Either or both
#   actions can be empty.
#
#   The METNO_ENABLE_CONDITIONAL macro defines an automake conditional
#   named ENABLE_<variable-prefix> which is true if and only if the
#   value of the enable_<option-name> shell variable is "yes".  This
#   is necessary because automake conditionals can not be defined
#   within conditional code.
#
# NOTES
#
#   The extra semicolons in the AS_IFs in the predicate macros are
#   there in case the respective action is empty; some shells croak if
#   there is nothing between "then" and "else" or between "else" and
#   "fi".
#
# TODO
#
#   Better help strings.
#
#   Better checking of the default value.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METNO_ENABLE
#
# $1 = variable-prefix
# $2 = option-name
# $3 = pretty-name
# $4 = default
#
AC_DEFUN([METNO_ENABLE], [
    AS_VAR_PUSHDEF([DEFAULT], m4_if([$4], [], [no], [$4]))
    AS_VAR_PUSHDEF([_ABLE], m4_if([DEFAULT], [yes], [disable], [enable]))
    AC_ARG_ENABLE([$2],
	AS_HELP_STRING([--_ABLE-$2], [_ABLE $3 @<:@default=DEFAULT@:>@]),
	[],
	[enable_$2=DEFAULT])
    AS_VAR_POPDEF([_ABLE])
    AS_VAR_POPDEF([DEFAULT])
])

#
# METNO_IF_ENABLED
#
# $1 = option-name
# $2 = action-if-enabled
# $3 = action-if-disabled
#
AC_DEFUN([METNO_IF_ENABLED], [
    AS_IF([test x${enable_$1} = xyes], [:;$2], [:;$3])
])

#
# METNO_IF_ENABLED_ANY
#
# $1 = option-names
# $2 = action-if-enabled
# $3 = action-if-disabled
#
AC_DEFUN([METNO_IF_ENABLED_ANY], [
    metno_if_enabled_any_result=no
    m4_foreach_w(COMPONENT, [$1], [
	AS_IF([test x${enable_[]COMPONENT} = xyes],
	    [metno_if_enabled_any_result=yes; break])
    ])
    AS_IF([test x${metno_if_enabled_any_result} = xyes], [:;$2], [:;$3])
    unset metno_if_enabled_any_result
])

#
# METNO_IF_ENABLED_ALL
#
# $1 = option-names
# $2 = action-if-enabled
# $3 = action-if-disabled
#
AC_DEFUN([METNO_IF_ENABLED_ALL], [
    metno_if_enabled_all_result=yes
    m4_foreach_w(COMPONENT, [$1], [
	AS_IF([test x${enable_[]COMPONENT} != xyes],
	    [metno_if_enabled_all_result=no; break])
    ])
    AS_IF([test x${metno_if_enabled_all_result} = xyes], [:;$2], [:;$3])
    unset metno_if_enabled_all_result
])

#
# METNO_ENABLE_CONDITIONAL
#
# $1 = variable-prefix
# $2 = option-name
#
AC_DEFUN([METNO_ENABLE_CONDITIONAL], [
    AM_CONDITIONAL([ENABLE_$1], [test x"${enable_$2}" = x"yes"])
])
