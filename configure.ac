AC_PREREQ([2.69])
AC_INIT([velocityper],[1.0.0],[phil@freebsd.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign -Wno-portability])

# Support silent build rules.  Requires at least automake-1.11.
# Disable with "configure --disable-silent-rules" or "make V=1"
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_INSTALL
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LN_S

# Must be after AC_PROG_AR
LT_INIT([dlopen shared])

AC_PATH_PROG(BASENAME, basename, /usr/bin/basename)
AC_PATH_PROG(BISON, bison, /usr/bin/bison)
AC_PATH_PROG(CAT, cat, /bin/cat)
AC_PATH_PROG(CHMOD, chmod, /bin/chmod)
AC_PATH_PROG(CP, cp, /bin/cp)
AC_PATH_PROG(DIFF, diff, /usr/bin/diff)
AC_PATH_PROG(MKDIR, mkdir, /bin/mkdir)
AC_PATH_PROG(MV, mv, /bin/mv)
AC_PATH_PROG(RM, rm, /bin/rm)
AC_PATH_PROG(SED, sed, /bin/sed)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_CHECK_FUNCS(err errx nanosleep strtoull)

dnl
dnl Some packages need to be checked against version numbers so we
dnl define a function here for later use
dnl
AC_DEFUN([VERSION_TO_NUMBER],
[`$1 | sed -e 's/lib.* //' | awk 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`])

LIBSLAX_CONFIG_PREFIX=""
LIBSLAX_SRC=""

AC_MSG_CHECKING([whether to build with warnings])
AC_ARG_ENABLE([warnings],
    [  --enable-warnings    Turn on compiler warnings],
    [VELOCITYPER_WARNINGS=$enableval],
    [VELOCITYPER_WARNINGS=no])
AC_MSG_RESULT([$VELOCITYPER_WARNINGS])
AM_CONDITIONAL([VELOCITYPER_WARNINGS_HIGH], [test "$VELOCITYPER_WARNINGS" != "no"])

AC_MSG_CHECKING([whether to build with debugging])
AC_ARG_ENABLE([debug],
    [  --enable-debug    Turn on debugging],
    [VELOCITYPER_DEBUG=yes; AC_DEFINE([VELOCITYPER_DEBUG], [1], [Enable debugging])],
    [VELOCITYPER_DEBUG=no])
AC_MSG_RESULT([$VELOCITYPER_DEBUG])
AM_CONDITIONAL([VELOCITYPER_DEBUG], [test "$VELOCITYPER_DEBUG" != "no"])

AC_MSG_CHECKING([compiler for gcc])
HAVE_GCC=no
if test "${CC}" != ""; then
    HAVE_GCC=`${CC} --version 2>&1 | grep -i GCC`
    if test "${HAVE_GCC}" != ""; then
        HAVE_GCC=yes
	AC_DEFINE([HAVE_GCC], [1], [Using real gcc])
    else
        HAVE_GCC=no
    fi
fi
AC_MSG_RESULT([$HAVE_GCC])
AM_CONDITIONAL([HAVE_GCC], [test "$HAVE_GCC" = "yes"])

case $host_os in
     darwin*)
        LIBTOOL=glibtool
	;;
     Linux*|linux*)
        CFLAGS="-D_GNU_SOURCE $CFLAGS"
	LDFLAGS=-ldl
	;;
     cygwin*|CYGWIN*)
	LDFLAGS=-no-undefined
	;;
esac

case $prefix in
     NONE)
	prefix=/usr/local
	;;
esac

dnl for the spec file
RELDATE=`date +'%Y-%m-%d%n'`
AC_SUBST(RELDATE)

AC_MSG_RESULT(Using configure dir $ac_abs_confdir)

if test -d $ac_abs_confdir/.git ; then
  extra=`git branch | awk '/\*/ { print $2 }'`
  if test "$extra" != "" -a "$extra" != "master"
  then
      VELOCITYPER_VERSION_EXTRA="-git-$extra"
  fi
fi

VELOCITYPER_VERSION=$PACKAGE_VERSION
VELOCITYPER_VERSION_NUMBER=VERSION_TO_NUMBER(echo $PACKAGE_VERSION)
AC_SUBST(VELOCITYPER_VERSION)
AC_SUBST(VELOCITYPER_VERSION_NUMBER)
AC_SUBST(VELOCITYPER_VERSION_EXTRA)

AC_DEFINE_UNQUOTED(VELOCITYPER_VERSION, ["$VELOCITYPER_VERSION"],
    [Version number as dotted value])
AC_DEFINE_UNQUOTED(VELOCITYPER_VERSION_NUMBER, [$VELOCITYPER_VERSION_NUMBER],
    [Version number as a number])
AC_DEFINE_UNQUOTED(VELOCITYPER_VERSION_STRING, ["$VELOCITYPER_VERSION_NUMBER"],
    [Version number as string])
AC_DEFINE_UNQUOTED(VELOCITYPER_VERSION_EXTRA, ["$VELOCITYPER_VERSION_EXTRA"],
    [Version number extra information])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  tests/Makefile
  velocityper.pc
  velocityper.spec
])
AC_OUTPUT

AC_MSG_NOTICE([summary of build options:

  velocityper version: ${VERSION} ${VELOCITYPER_VERSION_EXTRA}
  host type:           ${host} / ${host_os}
  install prefix:      ${prefix}

  compiler:            ${CC} (${HAVE_GCC:-no})
  compiler flags:      ${CFLAGS}
  library types:       Shared=${enable_shared}, Static=${enable_static}

  warnings:            ${VELOCITYPER_WARNINGS:-no}
  debug:               ${VELOCITYPER_DEBUG:-no}
])
