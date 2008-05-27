# $Id$
#include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

lib_LTLIBRARIES = libcommon_core.la libcommon_util.la
check_PROGRAMS  = check_string hanja

libcommon_core_la_SOURCES = common_core.c \
							setproctitle.c \
							memory.c \
							ipc.c \
							log_error.c \
                            config.c \
                            hook.c \
                            modules.c \
							registry.c \
							scoreboard.c \
							util.c \
							hangul.c \
							hanja.c
libcommon_core_la_LDFLAGS = -module -avoid-version -no-undefined
libcommon_core_la_LIBADD  = $(DYN_LIB) $(THREAD_LIB)

check_string_SOURCES = check_string.c
check_string_LDADD   = libcommon_core.la

hanja_SOURCES = hanja.c
hanja_LDADD   = libcommon_core.la
hanja_CPPFLAGS= -DENABLE_CHECK

libcommon_util_la_SOURCES = common_util.c \
							qsort2.c \
							msort.c \
							md5c.c \
							hash.c \
							memfile.c \
							mprintf.c \
							msg_record.c
libcommon_util_la_LDFLAGS = -module -avoid-version -no-undefined
libcommon_util_la_LIBADD = libcommon_core.la
