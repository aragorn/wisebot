include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

bin_PROGRAMS = ifs_defrag

ifs_defrag_SOURCES = ifs_defrag.c
ifs_defrag_LDADD = $(libdir)/libifs_defrag.la $(libdir)/libifs.la \
                   $(libdir)/libsfs.la $(libdir)/libapi.la \
				   $(libdir)/libcommon_core.la $(libdir)/libcommon_util.la

ifs_defrag_LDFLAGS = -L../lib -export-dynamic $(DYN_LIB) $(THREAD_LIB) $(SOFTBOTD_LDFLAGS)

include ../modules/install-pkglib.mk

