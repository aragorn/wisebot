# $Id$
include $(top_builddir)/Make.defs

SOURCE_FILES = mod_api.c mod_api.h \
			   lexicon.c lexicon.h \
			   cdm.c cdm.h \
			   did.c did.h \
			   vbm.c vbm.h \
			   tcp.c tcp.h \
			   udp.c udp.h \
			   qpp.c qpp.h \
			   qp.c qp.h \
			   vrfi.c vrfi.h \
			   indexer.c indexer.h \
			   docapi.c docapi.h \
			   protocol4.c protocol4.h \
			   tokenizer.c tokenizer.h \
			   morpheme.c morpheme.h \
			   xmlparser.c xmlparser.h \
			   rmas.c rmas.h \
			   index_word_extractor.c \
			   docattr.c docattr.h \
			   index_word_extractor.h \
               indexdb.c indexdb.h \
			   hash.c hash.h


lib_LTLIBRARIES = libapi.la
libapi_la_SOURCES = $(SOURCE_FILES)
if AIX5
libapi_la_LDFLAGS = -all-static -avoid-version
else
libapi_la_LDFLAGS = -shared -avoid-version
endif

pkglib_LTLIBRARIES = mod_api.la
mod_api_la_SOURCES = $(SOURCE_FILES)
mod_api_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
