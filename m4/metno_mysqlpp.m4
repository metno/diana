# -*- autoconf -*-
#
# metno_mysqlpp.m4    aleksandarb
#
# SYNOPSIS
#
#   METNO_REQUIRE_MYSQLPP(default)
#
# DESCRIPTION
#
#   The METNO_WITH_MYSQLPP macro searches for the mysqlpp library.
#
#   The METNO_REQUIRE_MYSQLPP macro is equivalent, but terminates the
#   configure script if it can not find the mysql++ library.
#
# AUTHORS
#
#   Aleksandar Babic aleksandarb@met.no
#

#
# METNO_WITH_MYSQLPP
#
# $1 = default
#
AC_DEFUN([METNO_WITH_MYSQLPP], [
    # --with-mysqlpp magic
    METNO_WITH_LIBRARY([MYSQLPP], [mysqlpp], [mysqlpp library], [$1])

    # is MYSQLPP required, or did the user request it?
    AS_IF([test x"${with_mysqlpp}" != x"no" -o x"${require_mysqlpp}" = x"yes"], [
	# save state
	saved_CPPFLAGS="${CPPFLAGS}"
	saved_LDFLAGS="${LDFLAGS}"
	saved_LIBS="${LIBS}"
	LIBS=""

	# header location
	AS_IF([test x"${MYSQLPP_INCLUDEDIR}" != x""], [
	    MYSQLPP_CPPFLAGS="-DMYSQLPP_MYSQL_HEADERS_BURIED -I${MYSQLPP_INCLUDEDIR}"
        ])
	CPPFLAGS="${CPPFLAGS} ${MYSQLPP_CPPFLAGS}"

	# library location
	AS_IF([test x"${MYSQLPP_LIBDIR}" != x""], [
	    MYSQLPP_LDFLAGS="-L${MYSQLPP_LIBDIR}"
            LIBS="-lmysqlpp"
	])
	LDFLAGS="${LDFLAGS} ${MYSQLPP_LDFLAGS}"

	# C++ version
	AC_LANG_PUSH(C++)
	AC_CHECK_HEADER([mysql++.h], [], [
	    AC_MSG_ERROR([the required mysql++.h header was not found])
	])
        AC_TRY_COMPILE([
                           #include <mysql++.h>
                       ],
                       [
                           mysqlpp::Connection dbconn_;
                       ],
                       [], 
                       [
	                   AC_MSG_ERROR([mysqlpp::sql_bigint was not found])
	               ])
	AC_LANG_POP(C++)

	# export our stuff
	MYSQLPP_LIBS="${LIBS}"
	AC_SUBST([MYSQLPP_LIBS])
	AC_SUBST([MYSQLPP_LDFLAGS])
	AC_SUBST([MYSQLPP_CPPFLAGS])

	# restore state
	LIBS="${saved_LIBS}"
	LDFLAGS="${saved_LDFLAGS}"
	CPPFLAGS="${saved_CPPFLAGS}"
    ])
])

#
# METNO_REQUIRE_MYSQLPP
#
AC_DEFUN([METNO_REQUIRE_MYSQLPP], [
    require_mysqlpp=yes
    METNO_WITH_MYSQLPP
])
