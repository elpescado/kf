dnl 
dnl kf - my configure.in script
dnl
dnl - autoconf is evil
dnl

AC_INIT(configure.ac)
AM_INIT_AUTOMAKE(kf, "0.5.4.1")
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

#DUPA=DUPAXXX

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC

PKG_PROG_PKG_CONFIG

dnl Optional Stuff


AM_BINRELOC

dnl Configuration Options
AC_ARG_ENABLE([gtkspell],	[  --enable-gtkspell    whether to enable gtkspell])
AC_ARG_ENABLE([debug],		[  --enable-debug       whether to enable debug])
dnl AC_ARG_ENABLE(gtkspell,)
dnl AC_ARG_ENABLE(debug,)

dnl modules needed for package
MODULES="gtk+-2.0 >= 2.0.0 libglade-2.0 >= 1.110.0 loudmouth-1.0 >= 0.16"
dnl MODULES="gtk+-2.0 >= 2.0.0 libglade-2.0 >= 1.110.0 loudmouth-1.0 >= 0.15.1"

dnl GtkSpell
if test "x$enable_gtkspell" == "xyes"
then
	PKG_CHECK_MODULES(GTKSPELL, gtkspell-2.0 >= 2.0.5, have_gtkspell="yes", have_gtkspell="no")
#	PACKAGE_LIBS="$PACKAGE_LIBS $GTKSPELL_LIBS"
#	PACKAGE_CFLAGS="$PACKAGE_CFLAGS $GTKSPELL_CFLAGS"
	if test "x$have_gtkspell" = "xyes"; then
		MODULES="$MODULES gtkspell-2.0 >= 2.0.5"
		AC_DEFINE([HAVE_GTKSPELL], [1], [Whether to support GtkSpell])
	fi
else
	have_gtkspell="no"
fi

if test "x$enable_debug" = "xyes"
then
	echo "debug..."
	AC_DEFINE([DEBUG],1,[Extra Debug])
fi

PKG_CHECK_MODULES(PACKAGE, [$MODULES])

dnl Check if we're using Loudmouth 0.17 or newer
LM17="loudmouth-1.0 >= 0.17";
PKG_CHECK_MODULES(LM, $LM17, have_lmjid="yes", have_lmjid="no")
if test "x$have_lmjid" = "xyes"
then
	AC_DEFINE([HAVE_LM_CONNECTION_SET_JID],1,[We have lm_connection_set_jid])
fi

dnl XScreenSaver stuff...
AC_DEFINE([HAVE_XSCREENSAVER], [1], [Whether to support XScreenSaver extension])
PACKAGE_LIBS="$PACKAGE_LIBS -lXss -L/usr/X11R6/lib"

AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

dnl AC_PROG_INTLTOOL
GETTEXT_PACKAGE=kf
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Gettext package name])

dnl Add the languages which your application supports here.
ALL_LINGUAS="bg pl ru"
AM_GLIB_GNU_GETTEXT

AC_OUTPUT([
	   Makefile
	   src/Makefile
	   data/Makefile
	   data/emoticons/Makefile
	   po/Makefile.in
	   kf.apspec
])

echo ""
echo "kf configuration:"
echo ""
echo "Location: $prefix"
echo "GtkSpell: $have_gtkspell"
echo "BinReloc: $br_cv_binreloc"
echo ""
echo "Now type 'make' to compile kf"

