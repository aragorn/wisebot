include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in debug_*

bin_PROGRAMS = debug_sb_fopen debug_sb_tstat

debug_sb_fopen_SOURCES = sb_fopen.c
debug_sb_fopen_LDFLAGS = $(DYN_LIB) $(THREAD_LIB) $(APR_LDFLAGS) $(APU_LDFLAGS)
debug_sb_fopen_LDADD = \
    $(APR_LIBS) $(APU_LIBS) \
    ../server/libsoftbotserver.la \
    ../platform/$(OS_TYPE)/libplatform.la

debug_sb_tstat_SOURCES = sb_tstat.c
debug_sb_tstat_LDFLAGS = $(DYN_LIB) $(THREAD_LIB) $(APR_LDFLAGS) $(APU_LDFLAGS)
debug_sb_tstat_LDADD = \
    $(APR_LIBS) $(APU_LIBS) \
    ../platform/$(OS_TYPE)/libplatform.la \
    ../server/libsoftbotserver.la
