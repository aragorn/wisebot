# $Id$
include $(top_builddir)/Make.defs

SOURCE_FILES = mod_api.c mod_api.h \
			   lexicon.c lexicon.h \
			   cdm.c cdm.h \
			   cdm2.c cdm2.h \
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
			   hash.c hash.h \
			   http_client.c http_client.h \
			   httpd.c httpd.h \
			   sbhandler.c sbhandler.h

SOURCE_FILES2 = mod_api.c mod_api.h \
			   lexicon.c lexicon.h \
			   cdm.c cdm.h \
			   did.c did.h \
			   vbm.c vbm.h \
			   tcp.c tcp.h \
			   udp.c udp.h \
			   qpp.c qpp.h \
			   qp2.c qp2.h \
			   vrfi.c vrfi.h \
			   indexer.c indexer.h \
			   docapi.c docapi.h \
			   protocol4.c protocol4.h \
			   tokenizer.c tokenizer.h \
			   morpheme.c morpheme.h \
			   xmlparser.c xmlparser.h \
			   rmas.c rmas.h \
			   index_word_extractor.c \
			   docattr2.c docattr2.h \
			   index_word_extractor.h \
               indexdb.c indexdb.h \
			   hash.c hash.h \
			   http_client.c http_client.h \
			   httpd.c httpd.h \
			   sbhandler.c sbhandler.h

# lib_LTLIBRARIES   = libapi.la
# libapi_la_SOURCES = $(SOURCE_FILES)
# libapi_la_CFLAGS  = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
# libapi_la_LDFLAGS = -shared -avoid-version -no-undefined
# libapi_la_LIBADD  = $(libdir)/libcommon_core.la $(APR_LIBS) $(APU_LIBS)

pkglib_LTLIBRARIES = mod_api.la mod_api2.la
mod_api_la_SOURCES = $(SOURCE_FILES)
mod_api_la_CFLAGS  = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
mod_api_la_LDFLAGS = -module -avoid-version -no-undefined
mod_api_la_LIBADD  = $(libdir)/libcommon_core.la $(APR_LIBS) $(APU_LIBS)

mod_api2_la_SOURCES = $(SOURCE_FILES2)
mod_api2_la_CFLAGS  = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
mod_api2_la_LDFLAGS = -module -avoid-version -no-undefined
mod_api2_la_LIBADD  = $(libdir)/libcommon_core.la $(APR_LIBS) $(APU_LIBS)

#include ../install-pkglib.mk
