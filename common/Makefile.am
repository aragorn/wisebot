# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

lib_LTLIBRARIES = libcommon_core.la libcommon_util.la

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
							util.c
libcommon_core_la_LDFLAGS = -shared -avoid-version -no-undefined


libcommon_util_la_SOURCES = common_util.c \
							qsort.c \
							qsort2.c \
							msort.c \
							md5c.c \
							hash.c \
							memfile.c \
							mprintf.c
libcommon_util_la_LDFLAGS = -shared -avoid-version
libcommon_util_la_LIBADD = libcommon_core.la
