# -*- autoconf -*-
#
AC_DEFUN([METNO_CHECK_QT4_TOOLS], [
    AC_REQUIRE([METNO_PROG_PKG_CONFIG])

    metno_qt_moc="`${PKG_CONFIG} --variable=moc_location $1`"
    AS_IF([test "x${metno_qt_moc}" = "x"], [
        AC_MSG_ERROR([cannot find Qt4 moc_location from pkg-config])
    ])
    AC_SUBST([MOC4], [${metno_qt_moc}])

    metno_qt_rcc="`${PKG_CONFIG} --variable=rcc_location $1`"
    AS_IF([test "x${metno_qt_rcc}" = "x"], [
        AC_MSG_ERROR([cannot find Qt4 rcc_location from pkg-config])
    ])
    AC_SUBST([RCC4], [${metno_qt_rcc}])

    metno_qt_uic="`${PKG_CONFIG} --variable=uic_location $1`"
    AS_IF([test "x${metno_qt_uic}" = "x"], [
        AC_MSG_ERROR([cannot find Qt4 uic_location from pkg-config])
    ])
    AC_SUBST([UIC4], [${metno_qt_uic}])

    metno_qt_lupdate="`${PKG_CONFIG} --variable=lupdate_location $1`"
    AS_IF([test "x${metno_qt_lupdate}" = "x"], [
        AC_MSG_ERROR([cannot find Qt4 lupdate_location from pkg-config])
    ])
    AC_SUBST([LUPDATE4], [${metno_qt_lupdate}])

    metno_qt_lrelease="`${PKG_CONFIG} --variable=lrelease_location $1`"
    AS_IF([test "x${metno_qt_lrelease}" = "x"], [
        AC_MSG_ERROR([cannot find Qt4 lrelease_location from pkg-config])
    ])
    AC_SUBST([LRELEASE4], [${metno_qt_lrelease}])
])
