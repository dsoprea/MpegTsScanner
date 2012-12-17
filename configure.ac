AC_INIT(tsscanner, 1.0.0)
AC_PROG_CC

AC_LANG(C)

libdvbpsi=1

# Make sure the LIBDVBPSI library is available.
AC_CHECK_LIB(dvbpsi, dvbpsi_PushPacket, [], [libdvbpsi=0])

# Make sure at least one of the LIBDVBPSI headers is available.
AC_CHECK_HEADER(dvbpsi/dvbpsi.h, [], [libdvbpsi=0])

if test $libdvbpsi == 0; then
	AC_MSG_ERROR([One or more basic libdvbpsi dependencies failed.])
fi

AC_OUTPUT(Makefile)

