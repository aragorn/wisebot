# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_lexicon.la mod_lexicon2.la

mod_lexicon_la_SOURCES = mod_lexicon.c \
						 lexicon_index_dynamic_hash.c
mod_lexicon_la_LDFLAGS = -module -avoid-version -no-undefined
mod_lexicon_la_LIBADD  = $(libdir)/libcommon_core.la

mod_lexicon2_la_SOURCES = mod_lexicon2.c
mod_lexicon2_la_LDFLAGS = -module -avoid-version -no-undefined
mod_lexicon2_la_LIBADD  = $(libdir)/libcommon_core.la 
						 
include ../install-pkglib.mk
