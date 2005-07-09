# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

SOURCE_FILES = mod_sfs.c super_block.c fat.c directory.c shared_memory.c \
               mod_sfs.h super_block.h fat.h directory.h shared_memory.h

lib_LTLIBRARIES = libsfs.la
libsfs_la_SOURCES = $(SOURCE_FILES)
if AIX5
libsfs_la_LDFLAGS = -all-static -avoid-version
else
libsfs_la_LDFLAGS = -shared -avoid-version
endif

pkglib_LTLIBRARIES = mod_sfs.la
mod_sfs_la_SOURCES = $(SOURCE_FILES)
mod_sfs_la_LDFLAGS = -module -avoid-version

check_PROGRAMS = test_sfs test2_sfs

test_sfs_SOURCES = test.c
test_sfs_LDADD = $(libdir)/libsfs.la $(libdir)/libapi.la $(libdir)/libsoftbot.la
test_sfs_LDFLAGS = -L$(libdir) -export-dynamic -static -ldl

test2_sfs_SOURCES = test2.c
test2_sfs_LDADD = $(libdir)/libsfs.la $(libdir)/libapi.la $(libdir)/libsoftbot.la
test2_sfs_LDFLAGS = -L$(libdir) -export-dynamic -static -ldl

include ../install-pkglib.mk
