# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_daemon_indexer.la \
                      mod_daemon_indexer2.la \
					  mod_index_each_doc.la \
					  mod_index_check.la

mod_daemon_indexer_la_SOURCES = mod_daemon_indexer.c mod_daemon_indexer.h hit.h
mod_daemon_indexer_la_LDFLAGS = -module -avoid-version

mod_daemon_indexer2_la_SOURCES = mod_daemon_indexer2.c mod_daemon_indexer.h hit.h
mod_daemon_indexer2_la_LDFLAGS = -module -avoid-version

mod_index_each_doc_la_SOURCES = mod_index_each_doc.c mod_index_each_doc.h
mod_index_each_doc_la_LDFLAGS = -module -avoid-version

mod_index_check_la_SOURCES = mod_index_check.c
mod_index_check_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
