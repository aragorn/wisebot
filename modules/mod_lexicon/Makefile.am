# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_lexicon.la 
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = mod_lexicon2.la
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_lexicon.la
check_LTLIBRARIES = mod_lexicon2.la
endif


mod_lexicon_la_SOURCES = mod_lexicon.c \
						 lexicon_index_dynamic_hash.c
mod_lexicon_la_LDFLAGS = $(MODULES_LDFLAGS)

mod_lexicon2_la_SOURCES = mod_lexicon2.c
mod_lexicon2_la_LDFLAGS = $(MODULES_LDFLAGS)
						 
include ../install-pkglib.mk
