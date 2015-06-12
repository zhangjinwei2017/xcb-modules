#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([xcb-modules], [0.0.1], [mengxiaoye@dce.com.cn])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/bs.c])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
# FIXME

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h strings.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday localtime_r pow sqrt strcasecmp strchr  \
strncasecmp strrchr])

AC_CONFIG_FILES([Makefile
                 conf/Makefile
                 src/Makefile])
AC_OUTPUT

echo "
######################################################################
($PACKAGE_NAME) Version $PACKAGE_VERSION
Prefix....: $prefix
Compiler..: $CC $CFLAGS $CPPFLAGS
Linker....: $LD $LDFLAGS $LIBS
######################################################################
"

