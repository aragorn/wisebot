# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_lexicon.la mod_lexicon2.la mod_lexicon3.la

mod_lexicon_la_SOURCES = mod_lexicon.c \
						 lexicon_index_dynamic_hash.c
mod_lexicon_la_LDFLAGS = -module -avoid-version 

mod_lexicon2_la_SOURCES = mod_lexicon2.c
mod_lexicon2_la_LDFLAGS = -module -avoid-version
						 
mod_lexicon3_la_SOURCES = mod_lexicon3.c \
						 lexicon_index_dynamic_hash.c
mod_lexicon3_la_LDFLAGS = -module -avoid-version 
include ../install-pkglib.mk
