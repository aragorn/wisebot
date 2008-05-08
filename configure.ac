# $Id$ 
# 
# Autoconf configuration fot SoftBot 6.0
#

AC_INIT([SoftBot],[6.0.0],[support@softwise.co.kr],[softbot-6.0.0])
PACKAGE=softbot

AC_PREREQ(2.50)
AC_COPYRIGHT([Copyright 2001-2005 SoftWise Inc.])
AC_REVISION([$Revision$])

AC_CONFIG_AUX_DIR([build])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign])
#AM_INIT_AUTOMAKE([foreign no-dependencies])
AC_CONFIG_HEADERS([include/auto_config.h])
dnl AC_CONFIG_HEADER(include/auto_config.h)

dnl XXX setting default prefix to `pwd` is not a good idea. --jiwon
dnl but, we can't avoid this for a time being. --aragorn
AC_PREFIX_DEFAULT(`pwd`)
AM_CONDITIONAL(PREFIX_IS_PWD,test $(prefix) -eq $(top_builddir))

dnl ---------------------------------------------------------------------------
dnl  APR and APU Stuff
dnl ---------------------------------------------------------------------------

  
echo $ac_n "${nl}Configuring Apache Portable Runtime library ...${nl}"

# find apr-config binary
AC_ARG_WITH(apr_config,
  AS_HELP_STRING([[--with-apr-config=FILE]],[Path to apr-config program]),
  [ apr_config="$withval" ],
  [AC_PATH_PROGS(apr_config,
                 [apr-config apr-0-config apr-1-config], 
                 [no], 
                 [$PATH:/usr/sbin/:/usr/local/apache2/bin]
                )]
)

echo apr version: `$apr_config --version`
if test "$apr_config" = "no"; then
   AC_MSG_ERROR(Could not find the apr-config program.  You can specify a location with the --with-apr-config=FILE option.  It may be named apr-0-config or apr-1-config and can be found in your apache2 bin directory.)
fi

APR_LIBS=`$apr_config --link-libtool --libs`
APR_INCLUDES=`$apr_config --includes`
APR_CFLAGS=`$apr_config --cflags`
APR_CPPFLAGS=`$apr_config --cppflags`
APR_LDFLAGS=`$apr_config --ldflags`
dnl APR_BINDIR=`apr-config --bindir`
dnl SHLIBPATH_VAR=`apr-config --shlib-path-var`


echo $ac_n "${nl}Configuring Apache Portable Runtime Utility library...${nl}"

# find apu-config binary
AC_ARG_WITH(apu_config,
  AS_HELP_STRING([[--with-apu-config=FILE]],[Path to apu-config program]),
  [ apu_config="$withval" ],
  [AC_PATH_PROGS(apu_config,
                 [apu-config apu-0-config apu-1-config], 
                 [no], 
                 [$PATH:/usr/sbin/:/usr/local/apache2/bin]
                )]
)

echo apu version: `$apu_config --version`
if test "$apu_config" = "no"; then
   AC_MSG_ERROR(Could not find the apu-config program.  You can specify a location with the --with-apu-config=FILE option.  It may be named apu-0-config or apu-1-config and can be found in your apache2 bin directory.)
fi

dnl XXX to test with new iconv
APU_LIBS=`$apu_config --link-libtool --libs`
APU_LDFLAGS=`$apu_config --ldflags`
APU_INCLUDES=`$apu_config --includes`
APU_BINDIR=`$apu_config --bindir`

dnl  APR and APU Stuff end.

my_libtool=`$apr_config --apr-libtool`
LIBTOOL="$my_libtool \$(LTFLAGS)"
libtoolversion=`$my_libtool --version`
case $libtoolversion in
	*1.[[45]]*)
		SH_LIBTOOL='$(LIBTOOL)'
		SHLTCLFAGS="-prefer-pic"
		AM_LIBTOOLFLAGS="--silent"
		LTCFLAGS="-prefer-non-pic -static"
		;;
	*)
		SH_LIBTOOL='$(SHELL) $(top_builddir)/shlibtool $(LTFLAGS)'
		SHLTCFLAGS=""
		AM_LIBTOOLFLAGS="--silent"
		LTCFLAGS=""
		;;
esac

dnl ---------------------------------------------------------------------------
dnl header template, include/auto_config.h.in

AH_TOP([#ifndef AUTO_CONFIG_H
#define AUTO_CONFIG_H 1

#include "auto_defines.h"

#define DEFAULT_CONFIG_FILE     "etc/softbot.conf"
#define DEFAULT_ERROR_LOG_FILE  "logs/error_log"
#define DEFAULT_QUERY_LOG_FILE  "logs/query_log"
#define DEFAULT_TIME_LOG_FILE   "logs/time_log"
#define DEFAULT_PID_FILE        "logs/softbotd.pid"
#define DEFAULT_REGISTRY_FILE   "dat/softbotd.reg"

#define CONFIG_PATH     "etc"
#define DB_PATH         "dat"
#define LOG_PATH        "logs"
#define LOCALSTATEDIR   "var"
#define DATADIR         "share"

/* whether to compile with DEBUG option */
#define DEBUG_SOFTBOT 1

])

AH_BOTTOM([
#endif /* AUTO_CONFIG_H */
])

AH_VERBATIM([1_NEAR_TOP__],[
/* checking system environment */
#undef LINUX
#undef LINUX_64
#undef CYGWIN
#undef FREEBSD
#undef SOLARIS
#undef AIX5
#undef HPUX

#undef SRCLIB_EXPAT
#undef SRCLIB_ICONV
#undef USR_LOCAL_INCLUDE_ICONV

/* whether indexer saves forward index or not. default is NOT using it. */
#define FORWARD_INDEX 0

/* whether to use apr, apr-util or not. default is NOT using it. */
#define USE_APR 0

/* hit size of hits of doc_hit_t. default size is 4 */
#define HIT_SIZE 4

/* whether to use timelog() or not.
   see include/timelog.h, platform/unix/sb_tstat.[ch]. */
#define USE_TIMELOG 0

/* whether to use daum_koma or not */
#define USE_DAUM_KOMA 0

/* whether to use pkoma or not */
#define USE_POSTECH_KOMA 0

/* If you don't have setproctitle, SB_ARGV_TYPE needs to be set to either
* SB_ARGV_NEW (replace argv[] arguments), SB_ARGV_WRITEABLE (overwrite
* argv[]), SB_ARGV_PSTAT (use the pstat function), or SB_ARGV_PSSTRINGS
* (use PS_STRINGS).
*
* configure should, we hope <wink>, detect this for you.
*/

#define SB_ARGV_NONE        0
#define SB_ARGV_NEW         1
#define SB_ARGV_WRITEABLE   2
#define SB_ARGV_PSTAT       3
#define SB_ARGV_PSSTRINGS   4

#undef SB_ARGV_TYPE

])





dnl ---------------------------------------------------------------------------

AC_ARG_WITH(forwardidx,
[  --with-forwardidx       build with forward index support [default=no]], [
if test "$withval" != yes
then
    AC_DEFINE(FORWARD_INDEX,0)
else
    AC_DEFINE(FORWARD_INDEX,1)
fi
])

AC_ARG_WITH(hitsize,
[  --with-hitsize          build with specified hit size,  [default=4]], [
AC_DEFINE_UNQUOTED(HIT_SIZE,$withval)
])

# AC_ARG_ENABLE(apr,
# [  --enable-apr            build with apr, apr-util support [default=no]], [
# if test "$enableval" != yes
# then
#     AC_DEFINE(USE_APR,0)
# else
#     AC_DEFINE(USE_APR,1)
# fi
# ])

AC_ARG_ENABLE(timelog,
[  --enable-timelog        use timelog().                  [default=no]], [
if test "$enableval" != yes
then
    AC_DEFINE(USE_TIMELOG,0)
else
    AC_DEFINE(USE_TIMELOG,1)
fi
])

AM_CONDITIONAL(USE_DAUM_KOMA,test 0 -eq 1)
AC_ARG_WITH(daum-koma,
[  --with-daum-koma        build with Daum's koma.          [default=no]], [
if test "$withval" = yes
then
    AM_CONDITIONAL(USE_DAUM_KOMA,test 1 -eq 1)
	AC_DEFINE(USE_DAUM_KOMA,1)
else
	AC_DEFINE(USE_DAUM_KOMA,0)
fi
])

AM_CONDITIONAL(USE_POSTECH_KOMA,test 0 -eq 1)
AC_ARG_WITH(postech-koma,
[  --with-postech-koma     build with Postech's koma        [default=no]], [
if test "$withval" = yes
then
    AM_CONDITIONAL(USE_POSTECH_KOMA,test 1 -eq 1)
	AC_DEFINE(USE_POSTECH_KOMA,1)
else
	AC_DEFINE(USE_POSTECH_KOMA,0)
fi
])

dnl ---------------------------------------------------------------------------

dnl See if the compiler supports __attribute__ gracefully.
AC_MSG_CHECKING(whether compiler supports __attribute__)
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[]], [[int x __attribute__((__unused__));]])],
  [compiler_supports_attribute=yes],
  [compiler_supports_attribute=no]
)
AC_MSG_RESULT($compiler_supports_attribute)
if test x"$compiler_supports_attribute" = xyes; then
    attr_unused="__attribute__((__unused__))"
else
    attr_unused=""
fi
AC_DEFINE_UNQUOTED(ATTR_UNUSED, $attr_unused,
                   [see if the compiler supports __attribute__ gracefully])

dnl ---------------------------------------------------------------------------

dnl Check system environments
CANONICAL_HOST="$host"
SHORT_HOST=""
MODULES_LDFLAGS="-module -avoid-version"
SOFTBOTD_LDFLAGS=""
SOFTBOTCLI_LDFLAGS=""
case "$host" in 
  x86_64-*-linux-gnu )
    SHORT_HOST="linux_64"
    AC_DEFINE(LINUX_64,1)
    THREAD_LIB="-lpthread"
    DYN_LIB="-ldl"
    #EXTRA_CFLAGS="-m32 -march=i686"
    ;;
  i?86-*-linux-gnu )
    SHORT_HOST="linux"
    # sets #define LINUX in auto_config.h
    AC_DEFINE(LINUX,1)
    THREAD_LIB="-lpthread"
    DYN_LIB="-ldl"
    EXTRA_CFLAGS="-m32 -march=i686"
    ;;
  *pc-cygwin* )
    SHORT_HOST="cygwin"
    AC_DEFINE(CYGWIN,1)
    MODULES_LDFLAGS="-module -avoid-version"
    THREAD_LIB=""
    DYN_LIB="" #"-dlopen"
    ;;
  *freebsd* )
    SHORT_HOST="freebsd"
    AC_DEFINE(FREEBSD,1)
    THREAD_LIB="-lc_r"
    DYN_LIB="-lc"
    ;;
  *-solaris* )
    SHORT_HOST="solaris"
    AC_DEFINE(SOLARIS,1)
    THREAD_LIB="-lpthread"
    DYN_LIB="-ldl"
    SOFTBOTD_LDFLAGS='-lsocket -lnsl'
    SOFTBOTCLI_LDFLAGS='-lsocket -lnsl'
    ;;
  *-ibm-aix5* )
    # powerpc-ibm-aix5.1.0.0
    SHORT_HOST="aix5"
    AC_DEFINE(AIX5,1)
    THREAD_LIB="-lc -lpthread"
    DYN_LIB="-Wl,-brtl -lld"
    EXTRA_CFLAGS="-fsigned-char"
    SOFTBOTD_LDFLAGS='-Wl,-bdynamic -Wl,-bE:$(top_builddir)/server/softbotd.exp -Wl,-bmaxdata:0x40000000 -L/usr/local/lib -liconv'
    SOFTBOTCLI_LDFLAGS='-Wl,-bdynamic -Wl,-bE:$(top_builddir)/client/softbotcli.exp -Wl,-bmaxdata:0x40000000 -L/usr/local/lib -liconv'
    LDFLAGS="-Wl,-brtl"
    ;;
  *-hpux* )
    SHORT_HOST="hpux"
    AC_DEFINE(HPUX,1)
    ;;
esac

# You can use if-condition in Makefile.am
AM_CONDITIONAL(LINUX,    test x$SHORT_HOST = xlinux)
AM_CONDITIONAL(LINUX_64, test x$SHORT_HOST = xlinux_64)
AM_CONDITIONAL(CYGWIN,   test x$SHORT_HOST = xcygwin)
AM_CONDITIONAL(FREEBSD,  test x$SHORT_HOST = xfreebsd)
AM_CONDITIONAL(SOLARIS,  test x$SHORT_HOST = xsolaris)
AM_CONDITIONAL(AIX5,     test x$SHORT_HOST = xaix5)
AM_CONDITIONAL(HPUX,     test x$SHORT_HOST = xhpux)

#########################################
# srclib installation

if test -f "include/expat/expat.h"; then
	AC_DEFINE(SRCLIB_EXPAT,1)
fi

if test -f "include/iconv/iconv.h"; then
	AC_DEFINE(SRCLIB_ICONV,1)
fi

if test -f "/usr/local/include/iconv.h"; then
	AC_DEFINE(USR_LOCAL_INCLUDE_ICONV,1)
fi

AM_CONDITIONAL(SRCLIB_EXPAT, test -f "include/expat/expat.h")
AM_CONDITIONAL(SRCLIB_ICONV, test -f "include/iconv/iconv.h")
AM_CONDITIONAL(USR_LOCAL_LIB_ICONV, test -f "/usr/local/lib/libiconv.la")

#########################################

dnl ---------------------------------------------------------------------------

dnl Checks for programs
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_LEX
AC_PROG_YACC

dnl Various OS checks that apparently set required flags
dnl AC_AIX


CFLAGS=""
AM_CFLAGS="-Wall -g3 -O3 $EXTRA_CFLAGS"
AM_CPPFLAGS="-I\$(top_builddir)/modules"
AM_LDFLAGS=""

AC_LIBTOOL_WIN32_DLL
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

dnl ---------------------------------------------------------------------------

AC_TYPE_SIGNAL
AC_TYPE_PID_T

dnl should fail configure when followings headers does not exist.
AC_CHECK_HEADERS( \
  inttypes.h string.h stdlib.h unistd.h fcntl.h getopt.h \
  sys/stat.h sys/mman.h \
  sys/time.h sys/times.h ctype.h getopt.h sys/types.h io.h \
  stdarg.h pthread.h \
)

AC_CHECK_FUNCS( \
  getenv \
  getopt \
  getopt_long \
  madvise \
  memalign \
  valloc \
  alloca \
  mmap \
  mergesort \
  getaddrinfo \
  setlinebuf \
)


dnl ---------------------------------------------------------------------------
dnl replacing setproctitle stuff.
dnl corresponding header template is included above.

AC_REPLACE_FUNCS(setproctitle)

if test "$ac_cv_func_setproctitle" = "yes"; then
  AC_DEFINE(SB_ARGV_TYPE, SB_ARGV_NONE)
else
  sb_argv_set="no"
  AC_CHECK_HEADERS([sys/pstat.h],have_pstat_h="yes",have_pstat_h="no")
  if test "$have_pstat_h" = "yes"; then
    AC_CHECK_FUNCS(pstat)

    if test "$ac_cv_func_pstat" = "yes"; then
      AC_DEFINE(SB_ARGV_TYPE, SB_ARGV_PSTAT)
    else
      AC_DEFINE(SB_ARGV_TYPE, SB_ARGV_WRITABLE)
    fi

    sb_argv_set="yes"
  fi

  if test "$sb_argv_set" = "no"; then
    AC_EGREP_HEADER([#define.*PS_STRINGS.*],sys/exec.h,
            have_psstrings="yes",have_psstrings="no")
    if test "$have_psstrings" = "yes"; then
      AC_DEFINE(SB_ARGV_TYPE, SB_ARGV_PSSTRINGS)
      sb_argv_set="yes"
    fi
  fi

  if test "$sb_argv_set" = "no"; then
    AC_CACHE_CHECK([whether __progname and __progname_full are available],
      pf_cv_var_progname,
      [AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
          [[extern char *__progname, *__progname_full;]],
          [[__progname = "foo"; __progname_full = "foo bar";]])],
        [pf_cv_var_progname="yes"],
        [pf_cv_var_progname="no"])]
    )

    if test "$pf_cv_var_progname" = "yes"; then
      AC_DEFINE(HAVE___PROGNAME, 1,
            [whether __progname and __progname__full are available])
    fi

    AC_CACHE_CHECK([which argv replacement method to use],
        pf_cv_argv_type,
        [AC_EGREP_CPP(YES_IS_DEFINED,
            [
#if defined(__GNU_HURD__)
  YES_IS_DEFINED
#endif
            ],
            pf_cv_argv_type="new",
            pf_cv_argv_type="writeable"
            )
        ])

    if test "$pf_cv_argv_type" = "new"; then
      AC_DEFINE(SB_ARGV_TYPE, SB_ARGV_NEW)
      sb_argv_set="yes"
    fi

    if test "$sb_argv_set" = "no"; then
      AC_DEFINE(SB_ARGV_TYPE, SB_ARGV_WRITABLE)
    fi
  fi
fi

dnl ---------------------------------------------------------------------------
# PACKAGE variable determines pkglib path.
# Unfortunately /usr/share/autoconf/autoconf/general.m4 set PACKAGE 
# to softbot-6.0.0 and we have to reassign the value without version number.
# --aragorn, 2005/02/22
PACKAGE=softbot
SB_RELEASE_DATE=`LANG=C date "+%a %b %d %T %Z %Y"`

AC_SUBST(PACKAGE)
AC_SUBST(SB_RELEASE_DATE)
AC_SUBST(CANONICAL_HOST)

AC_SUBST(LIBTOOL)

AC_SUBST(THREAD_LIB)
AC_SUBST(DYN_LIB)
AC_SUBST(MODULES_LDFLAGS)
AC_SUBST(SOFTBOTD_LDFLAGS)
AC_SUBST(SOFTBOTCLI_LDFLAGS)

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_LIBTOOLFLAGS)

AC_SUBST(APR_LIBS)
AC_SUBST(APR_INCLUDES)
AC_SUBST(APR_CFLAGS)
AC_SUBST(APR_CPPFLAGS)
AC_SUBST(APR_LDFLAGS)
AC_SUBST(APU_LIBS)
AC_SUBST(APU_INCLUDES)
AC_SUBST(APU_CFLAGS)
AC_SUBST(APU_LDFLAGS)

AC_CONFIG_FILES([
    include/auto_defines.h

    Make.defs
    Makefile

    include/Makefile
    common/Makefile
    server/Makefile
    client/Makefile
	util/Makefile
    etc/Makefile
    debug/Makefile
    srclib/Makefile

    modules/Makefile
    modules/mod_api/Makefile
    modules/mod_mp/Makefile

    modules/mod_tcp/Makefile
    modules/mod_udp/Makefile

    modules/mod_sfs/Makefile
    modules/mod_ifs/Makefile
    modules/mod_indexer/Makefile

    modules/mod_qpp/Makefile
    modules/mod_qp/Makefile

    modules/mod_tokenizer/Makefile
    modules/mod_morpheme/Makefile
    modules/mod_index_word_extractor/Makefile


    modules/mod_docapi/Makefile
    modules/mod_docattr/Makefile
    modules/mod_cdm/Makefile
    modules/mod_protocol4/Makefile
    modules/mod_softbot4/Makefile
    modules/mod_did/Makefile
    modules/mod_xmlparser/Makefile

    modules/mod_lexicon/Makefile

    modules/mod_site/Makefile
	modules/mod_site_general/Makefile

    modules/mod_httpd/Makefile
    modules/mod_httpd_handler/Makefile
    modules/mod_http_client/Makefile

    modules/mod_vrm/Makefile
    modules/mod_replication/Makefile
])
AC_OUTPUT
