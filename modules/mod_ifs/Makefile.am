# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_ifs.la mod_ifs_defrag.la mod_ifs_test.la
mod_ifs_la_SOURCES = mod_ifs.c mod_ifs.h table.c
mod_ifs_la_LDFLAGS = -module -avoid-version -no-undefined
mod_ifs_la_LIBADD  = $(libdir)/libcommon_core.la \
                     $(pkglibdir)/mod_api.la

mod_ifs_defrag_la_SOURCES = mod_ifs_defrag.c
mod_ifs_defrag_la_LDFLAGS = -module -avoid-version -no-undefined
mod_ifs_defrag_la_LIBADD  = $(libdir)/libcommon_core.la \
                            $(pkglibdir)/mod_api.la

mod_ifs_test_la_SOURCES = mod_ifs_test.c
mod_ifs_test_la_LDFLAGS = -module -avoid-version -no-undefined
mod_ifs_test_la_LIBADD  = $(libdir)/libcommon_core.la \
                          $(pkglibdir)/mod_api.la

check_PROGRAMS = test_ifs test2_ifs

test_ifs_SOURCES = test.c
test_ifs_LDADD = mod_ifs_defrag.la \
                 mod_ifs.la \
                 ../mod_sfs/mod_sfs.la \
                 $(pkglibdir)/mod_api.la \
                 $(libdir)/libcommon_core.la \
                 $(libdir)/libcommon_util.la
test_ifs_LDFLAGS = -export-dynamic -ldl $(THREAD_LIB)

test2_ifs_SOURCES = test2.c
test2_ifs_LDADD = mod_ifs_defrag.la \
                  mod_ifs.la \
                  ../mod_sfs/mod_sfs.la \
                  $(pkglibdir)/mod_api.la \
                  $(libdir)/libcommon_core.la \
                  $(libdir)/libcommon_util.la
test2_ifs_LDFLAGS = -export-dynamic -ldl $(THREAD_LIB)

include ../install-pkglib.mk
