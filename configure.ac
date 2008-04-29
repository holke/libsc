dnl
dnl sc configure.ac
dnl

AC_PREREQ([2.59])
AC_INIT([sc], [0.1], [ccgo@ices.utexas.edu])
AC_CONFIG_HEADER([src/sc_config.h])
AC_CONFIG_SRCDIR([src/sc.h])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AC_CONFIG_LIBOBJ_DIR(src)
AM_INIT_AUTOMAKE

# General settings.
AC_CANONICAL_HOST

# Checks for programs.
echo "o----------------------------------"
echo "| Checking for compilers"
echo "o----------------------------------"

ACX_MPI([icc gcc cc])
AC_PROG_RANLIB
AC_CHECK_PROG([HAVE_PROG_SPLINT], [splint], [yes])
AM_CONDITIONAL([HAVE_SPLINT], [test "$HAVE_PROG_SPLINT" = yes])

# Checks for libraries.
echo "o----------------------------------"
echo "| Checking for libraries"
echo "o----------------------------------"

dnl SC_REQUIRE_LIB([m], [fabs])
dnl SC_REQUIRE_LIB([z], [adler32_combine])

echo "o----------------------------------"
echo "| Checking headers and functions"
echo "o----------------------------------"

# Checks for header files.
dnl AC_HEADER_STDC
dnl AC_CHECK_HEADERS([ctype.h stdarg.h stddef.h stdint.h stdio.h stdlib.h])
dnl AC_CHECK_HEADERS([execinfo.h getopt.h libgen.h signal.h unistd.h])
dnl AC_CHECK_HEADERS([arpa/inet.h netinet/in.h zlib.h])

# Checks for functions.
dnl AC_CHECK_FUNCS([backtrace backtrace_symbols])
dnl SC_REQUIRE_FUNCS([memset pow sqrt floor])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

echo "o----------------------------------"
echo "| Results are"
echo "o----------------------------------"
echo "| CC:          $CC"
echo "| CFLAGS:      $CFLAGS"
echo "| CPP:         $CPP"
echo "| CPPFLAGS:    $CPPFLAGS"
echo "| LDFLAGS:     $LDFLAGS"
echo "| LIBS:        $LIBS"
echo "o----------------------------------"

# And the end.
AC_CONFIG_FILES([Makefile
                 src/Makefile
                ])

AC_OUTPUT