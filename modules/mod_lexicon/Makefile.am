# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

lib_LTLIBRARIES = libmodlexicon.la
libmodlexicon_la_SOURCES = mod_lexicon.c \
						 lexicon_index_dynamic_hash.c \
						 lexicon_index_truncation.c
libmodlexicon_la_LDFLAGS = -all-static -avoid-version

libexec_LTLIBRARIES = mod_lexicon.la 
mod_lexicon_la_SOURCES = mod_lexicon.c \
						 lexicon_index_dynamic_hash.c \
						 lexicon_index_truncation.c
mod_lexicon_la_LDFLAGS = -module -avoid-version 
						 
include ../install-libexec.mk
