# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_sfs.la
mod_sfs_la_SOURCES = mod_sfs.c super_block.c fat.c directory.c shared_memory.c \
                     mod_sfs.h super_block.h fat.h directory.h shared_memory.h
mod_sfs_la_LDFLAGS = -module -avoid-version

check_PROGRAMS = test_sfs test2_sfs

test_sfs_SOURCES = test.c
test_sfs_LDADD = mod_sfs.la $(libdir)/libapi.la $(libdir)/libsoftbotserver.la
test_sfs_LDFLAGS = -L$(libdir) -export-dynamic -static -ldl

test2_sfs_SOURCES = test2.c
test2_sfs_LDADD = mod_sfs.la $(libdir)/libapi.la $(libdir)/libsoftbotserver.la
test2_sfs_LDFLAGS = -L$(libdir) -export-dynamic -static -ldl

include ../install-libexec.mk
