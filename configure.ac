AC_PREREQ([2.65])
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

# Libtool stuff
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT
LTDL_INIT

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

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
