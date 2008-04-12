# $Id$

bin_PROGRAMS = softbotcli

softbotcli_SOURCES = commands.h commands.c \
                     benchmark.h benchmark.c \
                     client.h client.c \
                     static_modules.c 
softbotcli_CPPFLAGS = $(AM_CPPFLAGS) -I../modules/mod_api/
softbotcli_LDFLAGS = $(SOFTBOTCLI_LDFLAGS) -L$(libdir) -export-dynamic 

### 기본적인 library가 뒤에 와야 함.
### 즉 가장 많이 dependent한 libplatform.la가 제일 뒤로..
softbotcli_LDADD = ../modules/mod_api/mod_api.la \
                   $(libdir)/libcommon_util.la $(libdir)/libcommon_core.la \
                   $(DYN_LIB) $(THREAD_LIB) \
                   -lreadline -lncurses 

