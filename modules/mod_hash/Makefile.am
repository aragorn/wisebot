include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_bdb_hash.la

mod_bdb_hash_la_SOURCES = mod_bdb_hash.c
mod_bdb_hash_la_LDFLAGS = -module -avoid-version -ldb $(THREAD_LIB)

include ../install-libexec.mk

