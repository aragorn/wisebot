# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

lib_LTLIBRARIES = libifs.la libifs_defrag.la
libifs_la_SOURCES = mod_ifs.c table.c mod_ifs.h
if AIX5
libifs_la_LDFLAGS = -all-static -avoid-version
else
libifs_la_LDFLAGS = -shared -avoid-version
endif

libifs_defrag_la_SOURCES = mod_ifs_defrag.c
if AIX5
libifs_defrag_la_LDFLAGS = -all-static -avoid-version
else
libifs_defrag_la_LDFLAGS = -shared -avoid-version
endif

pkglib_LTLIBRARIES = mod_ifs.la mod_ifs_defrag.la mod_ifs_test.la
mod_ifs_la_SOURCES = mod_ifs.c table.c \
                     mod_ifs.h
mod_ifs_la_LDFLAGS = -module -avoid-version

mod_ifs_defrag_la_SOURCES = mod_ifs_defrag.c
mod_ifs_defrag_la_LDFLAGS = -module -avoid-version

mod_ifs_test_la_SOURCES = mod_ifs_test.c
mod_ifs_test_la_LDFLAGS = -module -avoid-version

check_PROGRAMS = test_ifs test2_ifs

test_ifs_SOURCES = test.c
test_ifs_LDADD = $(libdir)/libifs_defrag.la $(libdir)/libifs.la $(libdir)/libsfs.la $(libdir)/libapi.la $(libdir)/libmp.la $(libdir)/libplatform.la $(libdir)/libsoftbot.la
test_ifs_LDFLAGS = -L$(libdir) -export-dynamic -ldl $(THREAD_LIB)

test2_ifs_SOURCES = test2.c
test2_ifs_LDADD = $(libdir)/libifs_defrag.la $(libdir)/libifs.la $(libdir)/libsfs.la $(libdir)/libapi.la $(libdir)/libmp.la $(libdir)/libplatform.la $(libdir)/libsoftbot.la
test2_ifs_LDFLAGS = -L$(libdir) -export-dynamic -ldl $(THREAD_LIB)

include ../install-pkglib.mk
