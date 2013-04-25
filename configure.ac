dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([configure.ac])
AC_CANONICAL_TARGET([])
AC_GNU_SOURCE

dnl For Automake
PACKAGE=protobuf-nginx
VERSION=1.1

dnl Initialize automake stuff
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_LANG([C++])

dnl Look for protobuf headers and libs

AC_ARG_WITH(protobuf,
[  --with-protobuf[=DIR]     where to find Google's Protocol Buffers],,
  with_protobuf=yes)

if test "$with_protobuf" != no; then
  if test -d "$with_protobuf"; then
    CPPFLAGS="$CPPFLAGS -I$with_protobuf/include"
    LDFLAGS="$LDFLAGS -L$with_protobuf/lib"
  else
    AC_CHECK_HEADER([google/protobuf/stubs/common.h],,
    [AC_MSG_ERROR([protobuf headers are required.  Try using the --with-protobuf
 option.])])
  fi
fi

AC_CONFIG_FILES([Makefile nginx/Makefile protongx/Makefile])
AC_OUTPUT

echo ""
echo "Configure finished!"
echo ""
