# -*- autoconf -*-
#
# $Id: metlibs.m4 198 2010-02-09 20:55:53Z dages $
#
# SYNOPSIS
#
#   METLIBS_COMPONENT(name, enabled)
#
# DESCRIPTION
#
#   These macros were written specifically for metlibs, and will
#   require modification to work in any other context.
#
#   The METLIBS_COMPONENT macro defines a component in metlibs.  It
#   uses METNO_ENABLE to define a configure option for that component,
#   and if the component is enabled, it defines appropriate CFLAGS,
#   LDFLAGS and LDADD variables so other metlib components can use it,
#   as well as an automake conditional.
#
#   The --enable-all and --disable-all command-line options can be
#   used to enable or disable all components, but individual --enable
#   or --disable options will override them, so it possible to combine
#   options to, say, disable all but a few components without having
#   to explicitly enable or disable every single component.
#
#   The METLIBS_DEPEND macro registers a dependency between two
#   components.
#
#   The METLIBS_CHECK_DEPEND checks all registered dependencies.  It
#   will complain if a component is disabled while another component
#   that depends on it is enabled.  As a side effect, it also defines
#
# TODO
#
#   Rewrite as follows (XXX partly done 2010-01-20)
#
#   - METLIBS_COMPONENT stores component names as a space-separated
#     list in a shell variable.
#
#   - METLIBS_DEPEND does not check directly, but add components to a
#     list, e.g. diField_depends="libmi milib propoly puCtools
#     puTools".
#
#   - A new macro, METLIBS_CHECK performs all the checks.
#
#   - Use a separate variable-prefix argument like the METNO macros
#     do.
#
#   - Reduce the use of local variables (AS_VAR) to a minimum.
#
# AUTHORS
#
#   Dag-Erling Smørgrav <des@des.no>
#   For Systek / Kantega / met.no
#

#
# METLIBS_COMPONENT
#
# $1 = name
# $2 = default
#
AC_DEFUN([METLIBS_COMPONENT], [
    METLIBS_COMPONENTS="$1 ${METLIBS_COMPONENTS}"
    AS_VAR_PUSHDEF([NAME], [$1])
    AS_VAR_PUSHDEF([NAME_UC], m4_translit([$1], [a-z.-], [A-Z__]))
    AS_VAR_PUSHDEF([NAME_LC], m4_translit([$1], [A-Z.-], [a-z__]))
    AS_VAR_PUSHDEF([DEFAULT], m4_if([$2], [], [yes], [$2]))
    AS_IF([test x"${enable_[]NAME}" = x""], [
        AS_IF([test x"${enable_all}" != x], [
	    enable_[]NAME_LC="$enable_all"
	])
    ])
    METNO_ENABLE(NAME_UC, NAME, [NAME library], [DEFAULT])
    METNO_IF_ENABLED(NAME, [
	AC_MSG_NOTICE([NAME component enabled])
	AC_SUBST(NAME_UC[_CPPFLAGS], ['-I$(top_srcdir)'])
	AC_SUBST(NAME_UC[_LIBS], ['$(top_builddir)/NAME/lib]NAME[.la'])
    ], [
	AC_MSG_NOTICE([NAME component disabled])
    ])
    METNO_ENABLE_CONDITIONAL(NAME_UC, NAME)
    AC_SUBST([$1_pc_requires])
    AS_VAR_POPDEF([DEFAULT])
    AS_VAR_POPDEF([NAME_LC])
    AS_VAR_POPDEF([NAME_UC])
    AS_VAR_POPDEF([NAME])
])

#
# METLIBS_DEPEND
#
# $1 = component-name
# $2 = dependency-list
#
AC_DEFUN([METLIBS_DEPEND], [
    METLIBS_$1_DEPENDS="$2 ${METLIBS_$1_DEPENDS}"
])

#
# METLIBS_CHECK_DEPEND
#
AC_DEFUN([METLIBS_CHECK_DEPEND], [
    for component in ${METLIBS_COMPONENTS} ; do
        AC_MSG_CHECKING([internal dependencies for ${component}])
	METNO_IF_ENABLED([$component], [
	    for dependency in `eval "echo \\${METLIBS_${component}_DEPENDS}"` ; do
		METNO_IF_ENABLED([$dependency], [
		    # OK
		], [
		    AC_MSG_RESULT([failed])
		    AC_MSG_ERROR([$component requires $dependency, which is disabled])
		])
		eval "
if test x\"\${${component}_pc_requires}\" = x\"\" ; then
    ${component}_pc_requires=\"${dependency} >= ${PACKAGE_VERSION}\" ;
else
    ${component}_pc_requires=\"\${${component}_pc_requires}, ${dependency} >= ${PACKAGE_VERSION}\" ;
fi
"
	    done
	])
        AC_MSG_RESULT([ok])
    done
])

#
# METLIBS_CONFIG_FILES
#
# Completely useless right now - need to reconsider once the tree is
# flattened
#
AC_DEFUN([METLIBS_CONFIG_FILES], [
    AC_CONFIG_FILES($2)
])
