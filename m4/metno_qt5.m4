# -*- autoconf -*-
#
AC_DEFUN([METNO_CHECK_QT5_TOOLS], [
    AC_REQUIRE([METNO_PROG_PKG_CONFIG])

    metno_qt_host_bins="`${PKG_CONFIG} --variable=host_bins Qt5Core`"
    AS_IF([test "x${metno_qt_host_bins}" = "x"], [
        AC_MSG_ERROR([cannot find Qt5 host_bins from pkg-config])
    ])
    AC_SUBST([MOC4],      [${metno_qt_host_bins}/moc])
    AC_SUBST([RCC4],      [${metno_qt_host_bins}/rcc])
    AC_SUBST([UIC4],      [${metno_qt_host_bins}/uic])
    AC_SUBST([LUPDATE4],  [${metno_qt_host_bins}/lupdate])
    AC_SUBST([LRELEASE4], [${metno_qt_host_bins}/lrelease])
])
