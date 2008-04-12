# $Id$

bin_PROGRAMS = ifs_defrag sort_test

ifs_defrag_SOURCES = ifs_defrag.c
ifs_defrag_LDADD = ../modules/mod_api/mod_api2.la \
				   $(libdir)/libcommon_util.la $(libdir)/libcommon_core.la \
                   $(DYN_LIB) $(THREAD_LIB) 
ifs_defrag_LDFLAGS = $(SOFTBOTD_LDFLAGS) -L$(libdir) -export-dynamic

sort_test_SOURCES = sort_test.c
sort_test_LDADD = ../modules/mod_api/mod_api2.la \
				  $(libdir)/libcommon_util.la $(libdir)/libcommon_core.la \
                  $(DYN_LIB) $(THREAD_LIB) 
sort_test_LDFLAGS = $(SOFTBOTD_LDFLAGS) -L$(libdir) -export-dynamic 

