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

# Checks for libraries.
AC_CHECK_LIB([m], [powl])
# FIXME: Replace `main' with a function in `-lpopulation':
#AC_CHECK_LIB([population], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdint.h stdlib.h string.h strings.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset strcasecmp strchr strdup strtol])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
