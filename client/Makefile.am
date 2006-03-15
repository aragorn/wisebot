# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

bin_PROGRAMS = softbotcli

softbotcli_SOURCES = commands.h commands.c \
                     benchmark.h benchmark.c \
                     client.h client.c \
                     static_modules.c 

softbotcli_LDFLAGS = $(SOFTBOTCLI_LDFLAGS) $(DYN_LIB) $(THREAD_LIB) \
                     -lreadline -lncurses -export-dynamic 

softbotcli_CFLAGS  = -I../modules/mod_api/

### 기본적인 library가 뒤에 와야 함.
### 즉 가장 많이 dependent한 libplatform.la가 제일 뒤로..
softbotcli_LDADD = $(libdir)/libcommon_core.la $(libdir)/libcommon_util.la \
				   $(libdir)/libapi.la

