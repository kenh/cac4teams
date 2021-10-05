dnl
dnl Our configure.ac template
dnl Process with autoconf (or better yet, autoreconf) to get configure
dnl

AC_PREREQ([2.69])

AC_INIT([cac4teams], [0.8.2], [kenh@cmf.nrl.navy.mil])
AC_CONFIG_SRCDIR([src/cac4teams.c])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects 1.16])
AC_PROG_CC([clang])
AM_PROG_AR

LT_PREREQ([2.4.6])
LT_INIT([disable-static dlopen])

default_APPIDENTIFIER="mil.navy.nrl.cmf.cac4teams"
AC_ARG_VAR([APPIDENTIFIER],
	   [Application bundle identifier of package, defaults to 'mil.navy.nrl.cmf.cac4teams'])
KH_SIGNING

: "${APPIDENTIFIER=$default_APPIDENTIFIER}"

AC_DEFINE_UNQUOTED([APPIDENTIFIER], ["$APPIDENTIFIER"],
	[Application bundle identifier of package])
AC_SUBST([APPIDENTIFIER])

AC_CONFIG_FILES([Makefile packaging/distribution.xml]
		[packaging/resources/welcome.rtf:packaging/resources/welcome-in.rtf])
AC_OUTPUT
