# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_daemon_indexer2.la mod_index_each_doc.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = mod_daemon_indexer.la mod_index_check.la
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_daemon_indexer2.la mod_index_each_doc.la
check_LTLIBRARIES = mod_daemon_indexer.la mod_index_check.la
endif

mod_daemon_indexer_la_SOURCES = mod_daemon_indexer.c mod_daemon_indexer.h
mod_daemon_indexer_la_LDFLAGS = $(MODULES_LDFLAGS)

mod_daemon_indexer2_la_SOURCES = mod_daemon_indexer2.c mod_daemon_indexer.h
mod_daemon_indexer2_la_LDFLAGS = $(MODULES_LDFLAGS)

mod_index_each_doc_la_SOURCES = mod_index_each_doc.c mod_index_each_doc.h
mod_index_each_doc_la_LDFLAGS = $(MODULES_LDFLAGS)

mod_index_check_la_SOURCES = mod_index_check.c
mod_index_check_la_LDFLAGS = $(MODULES_LDFLAGS)

include ../install-pkglib.mk
