include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

bin_PROGRAMS = ifs_defrag

ifs_defrag_SOURCES = ifs_defrag.c
ifs_defrag_LDADD = ../modules/mod_ifs/mod_ifs_defrag.la ../modules/mod_ifs/mod_ifs.la \
                   ../modules/mod_sfs/mod_sfs.la ../modules/mod_api/libapi.la \
				   ../modules/mod_mp/libmp.la ../lib/libplatform.la ../server/libsoftbotserver.la
ifs_defrag_LDFLAGS = -L../lib -export-dynamic -ldl $(THREAD_LIB)

include ../modules/install-libexec.mk

