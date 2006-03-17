# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

bin_PROGRAMS = softbotd
softbotd_SOURCES = static_modules.c \
					server.c server.h \
					mp_api.c mp_api.h
softbotd_LDADD   = $(libdir)/libcommon_core.la $(libdir)/libcommon_util.la
softbotd_LDFLAGS = $(SOFTBOTD_LDFLAGS) $(DYN_LIB) $(THREAD_LIB) -L../lib -export-dynamic
softbotd_CFLAGS  = 

lib_LTLIBRARIES = libmp.la

libmp_la_SOURCES = mp_api.c mp_api.h
libmp_la_LDFLAGS = -shared -avoid-version -no-undefined
# softbotd_LDFLAGS에 -export-dynamic 이 있어야 module들이 executable의 symbol을
# resolve할 수 있다. 이 옵션이 없으면, undefined symbol 에러로 mod_mp.so 를 load
# 하지 못한다. --김정겸, 2006/03/15
