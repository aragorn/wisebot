# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_lexicon.la mod_lexicon2.la

mod_lexicon_la_SOURCES = mod_lexicon.c \
						 lexicon_index_dynamic_hash.c
mod_lexicon_la_LDFLAGS = -module -avoid-version 

mod_lexicon2_la_SOURCES = mod_lexicon2.c
mod_lexicon2_la_LDFLAGS = -module -avoid-version
						 
include ../install-libexec.mk
