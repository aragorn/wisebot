include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in debug_*

bin_PROGRAMS = debug_sb_fopen debug_sb_tstat

debug_sb_fopen_SOURCES = sb_fopen.c
debug_sb_fopen_LDFLAGS = -L../lib $(DYN_LIB) $(THREAD_LIB) $(SOFTBOTD_LDFLAGS)
debug_sb_fopen_LDADD = $(libdir)/libcommon_core.la $(libdir)/libcommon_util.la

debug_sb_tstat_SOURCES = sb_tstat.c
debug_sb_tstat_LDFLAGS = -L../lib $(DYN_LIB) $(THREAD_LIB) $(SOFTBOTD_LDFLAGS)
debug_sb_tstat_LDADD = $(libdir)/libcommon_core.la $(libdir)/libcommon_util.la
