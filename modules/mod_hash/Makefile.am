include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_bdb_hash.la

mod_bdb_hash_la_SOURCES = mod_bdb_hash.c
mod_bdb_hash_la_LDFLAGS = -module -avoid-version -no-undefined
mod_bdb_hash_la_LIBADD = $(libdir)/libcommon_core.la \
                         $(libdir)/libdb-4.3.la

include ../install-pkglib.mk

