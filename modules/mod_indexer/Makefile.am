# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_daemon_indexer.la \
                      mod_daemon_indexer2.la \
					  mod_index_each_doc.la \
					  mod_index_check.la
#					  mod_pile_doc.la \
#					  mod_spooled_indexer.la \
#					  mod_index_each_spooled_doc.la \

#					  mod_index_doc_cdmi.la \
#					  mod_indexer_cdmi.la \

#mod_spooled_indexer_la_SOURCES = mod_spooled_indexer.c mod_spooled_indexer.h \
#								 hit.h
#mod_spooled_indexer_la_LDFLAGS = -module -avoid-version

#mod_index_each_spooled_doc_la_SOURCES = mod_index_each_spooled_doc.c mod_index_each_spooled_doc.h
#mod_index_each_spooled_doc_la_LDFLAGS = -module -avoid-version

#mod_pile_doc_la_SOURCES = mod_pile_doc.c mod_pile_doc.h
#mod_pile_doc_la_LDFLAGS = -module -avoid-version

mod_daemon_indexer_la_SOURCES = mod_daemon_indexer.c mod_daemon_indexer.h hit.h
mod_daemon_indexer_la_LDFLAGS = -module -avoid-version

mod_daemon_indexer2_la_SOURCES = mod_daemon_indexer2.c mod_daemon_indexer.h hit.h
mod_daemon_indexer2_la_LDFLAGS = -module -avoid-version

mod_index_each_doc_la_SOURCES = mod_index_each_doc.c mod_index_each_doc.h
mod_index_each_doc_la_LDFLAGS = -module -avoid-version

mod_index_check_la_SOURCES = mod_index_check.c
mod_index_check_la_LDFLAGS = -module -avoid-version

# cdmi
#mod_index_doc_cdmi_la_SOURCES = mod_index_doc_cdmi.c mod_index_doc_cdmi.h
#mod_index_doc_cdmi_la_LDFLAGS = -module -avoid-version

#mod_indexer_cdmi_la_SOURCES = mod_indexer_cdmi.c hit.h mod_indexer.h 
#mod_indexer_cdmi_la_LDFLAGS = -module -avoid-version

include ../install-libexec.mk
