# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_sfs.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_sfs.la
check_LTLIBRARIES = 
endif

SOURCE_FILES = mod_sfs.c super_block.c fat.c directory.c shared_memory.c \
               mod_sfs.h super_block.h fat.h directory.h shared_memory.h

mod_sfs_la_SOURCES = $(SOURCE_FILES)
mod_sfs_la_LDFLAGS = -module $(MODULES_LDFLAGS)

check_PROGRAMS = test_sfs test2_sfs

test_sfs_SOURCES = test.c
test_sfs_LDADD   = $(libdir)/libcommon_core.la \
                   mod_sfs.la
test_sfs_LDFLAGS = -export-dynamic -static -ldl

test2_sfs_SOURCES = test2.c
test2_sfs_LDADD   = $(libdir)/libcommon_core.la \
                    mod_sfs.la
test2_sfs_LDFLAGS = -export-dynamic -static -ldl

include ../install-pkglib.mk
