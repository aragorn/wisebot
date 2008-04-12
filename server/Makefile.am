# $Id$

bin_PROGRAMS = softbotd
softbotd_SOURCES = static_modules.c \
                   server.c server.h
softbotd_LDADD   = $(libdir)/libcommon_core.la $(libdir)/libcommon_util.la \
                   $(DYN_LIB) $(THREAD_LIB)
softbotd_LDFLAGS = $(SOFTBOTD_LDFLAGS) -L$(libdir) -export-dynamic

# softbotd_LDFLAGS에 -export-dynamic 이 있어야 module들이 executable의 symbol을
# resolve할 수 있다. 이 옵션이 없으면, undefined symbol 에러로 mod_mp.so 를 load
# 하지 못한다. --김정겸, 2006/03/15
