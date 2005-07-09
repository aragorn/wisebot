# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

noinst_PROGRAMS = test_msort

lib_LTLIBRARIES = libplatform.la
libplatform_la_SOURCES = init_set_proc_title.c \
					../generic/connect_nonb.c ../generic/connect_nonb.h \
					../generic/qsort.c \
					../generic/qsort2.c ../generic/qsort2.h \
					../generic/msort.c ../generic/msort.h \
					../generic/queue.c ../generic/queue.h \
					../generic/sz_string.c ../generic/sz_string.h \
					../generic/str_misc.c ../generic/str_misc.h \
					../generic/md5_global.h ../generic/md5.h ../generic/md5c.c \
					sb_fopen.c sb_open.c \
					flock.c flock.h \
					sb_lockfile.c sb_lockfile.h \
					setproctitle.c \
					os-aix-dso.c \
					hash.c \
					sb_tstat.c
if AIX5
libplatform_la_LDFLAGS = -avoid-version -static
else
libplatform_la_LDFLAGS = -avoid-version -shared -no-undefined
endif
# libplatform_la_LIBADD = $(top_builddir)/server/libsoftbot.la

test_msort_SOURCES = test_msort.c ../generic/msort.c
test_msort_LDFLAGS = $(DYN_LIB)
test_msort_CFLAGS  = $(AM_CFLAGS)

noinst_HEADERS = sb_fopen.h sb_open.h sb_tstat.h setproctitle.h

# currently we do not use LTLIBOBJS instead
# in setproctitle.c(and etc..) we wrap function setproctitle() 
# with #ifndef HAVE_SETPROCTITLE #endif.
# for platforms which has setproctitle function
#__libdir__libplatform_la_LIBADD = @LTLIBOBJS@			
