# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

SOURCE_FILES = mod_sfs.c super_block.c fat.c directory.c shared_memory.c \
               mod_sfs.h super_block.h fat.h directory.h shared_memory.h

pkglib_LTLIBRARIES = mod_sfs.la
mod_sfs_la_SOURCES = $(SOURCE_FILES)
mod_sfs_la_LDFLAGS = -module -avoid-version -no-undefined
mod_sfs_la_LIBADD  = $(libdir)/libcommon_core.la \
                     $(pkglibdir)/mod_api.la

check_PROGRAMS = test_sfs test2_sfs

test_sfs_SOURCES = test.c
test_sfs_LDADD   = $(libdir)/libcommon_core.la \
                   $(pkglibdir)/mod_api.la \
                   mod_sfs.la
test_sfs_LDFLAGS = -export-dynamic -static -ldl

test2_sfs_SOURCES = test2.c
test2_sfs_LDADD   = $(libdir)/libcommon_core.la \
                    $(pkglibdir)/mod_api.la \
                    mod_sfs.la
test2_sfs_LDFLAGS = -export-dynamic -static -ldl

#mod_sfs.la is needed by ifs_defrag
include ../install-pkglib.mk
