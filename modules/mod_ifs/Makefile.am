# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_ifs.la mod_ifs_defrag.la mod_ifs_test.la
mod_ifs_la_SOURCES = mod_ifs.c table.c \
                     mod_ifs.h
mod_ifs_la_LDFLAGS = -module -avoid-version

mod_ifs_defrag_la_SOURCES = mod_ifs_defrag.c
mod_ifs_defrag_la_LDFLAGS = -module -avoid-version

mod_ifs_test_la_SOURCES = mod_ifs_test.c
mod_ifs_test_la_LDFLAGS = -module -avoid-version

check_PROGRAMS = test_ifs test2_ifs

test_ifs_SOURCES = test.c
test_ifs_LDADD = mod_ifs_defrag.la mod_ifs.la ../mod_sfs/mod_sfs.la ../../lib/libapi.la ../mod_mp/libmp.la ../../lib/libplatform.la ../../lib/libsoftbotserver.la
test_ifs_LDFLAGS = -L../../lib -export-dynamic -ldl $(THREAD_LIB)

test2_ifs_SOURCES = test2.c
test2_ifs_LDADD = mod_ifs_defrag.la mod_ifs.la ../mod_sfs/mod_sfs.la ../../lib/libapi.la ../mod_mp/libmp.la ../../lib/libplatform.la ../../lib/libsoftbotserver.la
test2_ifs_LDFLAGS = -L../../lib -export-dynamic -ldl $(THREAD_LIB)

include ../install-libexec.mk
