# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_bdb_hash.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_bdb_hash.la
endif

mod_bdb_hash_la_SOURCES = mod_bdb_hash.c
mod_bdb_hash_la_LDFLAGS = $(MODULES_LDFLAGS)
mod_bdb_hash_la_LIBADD = $(libdir)/libcommon_core.la \
                         $(libdir)/libdb-4.3.la

include ../install-pkglib.mk

