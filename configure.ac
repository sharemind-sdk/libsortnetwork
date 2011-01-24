AC_PREREQ([2.61])
AC_INIT([libsortnetwork], [1.0.0], [ff@octo.it])
AC_CONFIG_SRCDIR([src/sn_network.c])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE(dist-bzip2)
AC_LANG(C)

AC_PREFIX_DEFAULT("/opt/libsortnetwork")

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

# Libtool stuff
LT_INIT

if test "x$GCC" = "xyes"
then
	CFLAGS="$CFLAGS -std=c99"
fi

AC_DEFINE([_ISOC99_SOURCE], [1], [Define to use C99])
AC_DEFINE([_POSIX_C_SOURCE], [200809L], [Set to desired POSIX version])
AC_DEFINE([_XOPEN_SOURCE], [700], [Set to desired X/Open version])

# TODO: Write check for libpopulation
#AC_CHECK_LIB([population], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdint.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_TYPE_INT64_T
#AC_TYPE_SIZE_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset strcasecmp strchr strdup strtol])

AC_ARG_VAR([LIBPOPULATION_CPPFLAGS], [C preprocessor flags required to build with libpopulation])
AC_ARG_VAR([LIBPOPULATION_LDFLAGS],  [Linker flags required to build with libpopulation])
AC_ARG_VAR([LIBPOPULATION_LIBS],     [Other libraries required to link against libpopulation])

# --with-libpopulation {{{
LIBPOPULATION_CPPFLAGS="$LIBPOPULATION_CPPFLAGS"
LIBPOPULATION_LDFLAGS="$LIBPOPULATION_LDFLAGS"
LIBPOPULATION_LIBS="$LIBPOPULATION_LIBS"
AC_ARG_WITH(libpopulation, [AS_HELP_STRING([--with-libpopulation@<:@=PREFIX@:>@], [Path to libpopulation.])],
[
	if test "x$withval" = "xyes" || test "x$withval" = "xno"
	then
		with_libpopulation="$withval"
	else
		LIBPOPULATION_CPPFLAGS="$LIBPOPULATION_CPPFLAGS -I$withval/include"
		LIBPOPULATION_LDFLAGS="$LIBPOPULATION_LDFLAGS -L$withval/lib"
		with_libpopulation="yes"
	fi
],
[
	with_libpopulation="yes"
])

SAVE_CPPFLAGS="$CPPFLAGS"
SAVE_LDFLAGS="$LDFLAGS"
CPPFLAGS="$CPPFLAGS $LIBPOPULATION_CPPFLAGS"
LDFLAGS="$LDFLAGS $LIBPOPULATION_LDFLAGS"

if test "x$with_libpopulation" = "xyes"
then
	AC_CHECK_HEADERS(population.h,
			 [with_libpopulation="yes"],
			 [with_libpopulation="no"])
fi

if test "x$with_libpopulation" = "xyes"
then    
	if test "x$LIBPOPULATION_LIBS" = "x"
	then
		LIBPOPULATION_LIBS="-lpthread"
	fi

	AC_CHECK_LIB(population, population_create,
		     [with_libpopulation="yes"],
		     [with_libpopulation="no"],
		     [$LIBPOPULATION_LIBS])
	LIBPOPULATION_LIBS="-lpopulation $LIBPOPULATION_LIBS"
fi      

CPPFLAGS="$SAVE_CPPFLAGS"
LDFLAGS="$SAVE_LDFLAGS"

if test "x$with_libpopulation" = "xyes"
then
	AC_DEFINE(HAVE_LIBPOPULATION, 1, [Define to 1 if you have the population library (-lpopulation).])
fi

AC_SUBST(LIBPOPULATION_CPPFLAGS)
AC_SUBST(LIBPOPULATION_LDFLAGS)
AC_SUBST(LIBPOPULATION_LIBS)
AM_CONDITIONAL(BUILD_WITH_LIBPOPULATION, test "x$with_libpopulation" = "xyes")
# }}} --with-libpopulation

PKG_CHECK_MODULES([glib], [glib-2.0],
		  [have_libglib="yes"],
		  [have_libglib="no"])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

# vim: set fdm=marker :
