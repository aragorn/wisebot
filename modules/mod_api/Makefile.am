# $Id$
include $(top_builddir)/Make.defs

noinst_LIBRARIES = libapi_common.a
libapi_common_a_SOURCES = \
				mod_api.c mod_api.h \
				lexicon.c lexicon.h \
				cdm.c cdm.h \
				cdm2.c cdm2.h \
				did.c did.h \
				vbm.c vbm.h \
				tcp.c tcp.h \
				udp.c udp.h \
				qpp.c qpp.h \
				vrfi.c vrfi.h \
				indexer.c indexer.h \
				docapi.c docapi.h \
				protocol4.c protocol4.h \
				tokenizer.c tokenizer.h \
				morpheme.c morpheme.h \
				xmlparser.c xmlparser.h \
				rmas.c rmas.h \
				index_word_extractor.c \
				index_word_extractor.h \
				indexdb.c indexdb.h \
				hash.c hash.h \
				http_client.c http_client.h \
				httpd.c httpd.h \
				sbhandler.c sbhandler.h
libapi_common_a_CFLAGS  = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
#libapi_common_a_LDFLAGS = -static 

SOURCE_FILES1 = qp.c qp.h \
				docattr.c docattr.h

SOURCE_FILES2 = qp2.c qp2.h \
				docattr2.c docattr2.h

pkglib_LTLIBRARIES = mod_api.la mod_api2.la
mod_api_la_SOURCES = $(SOURCE_FILES1)
mod_api_la_LDFLAGS = -module -avoid-version -no-undefined
mod_api_la_LIBADD  = libapi_common.a $(libdir)/libcommon_core.la $(APR_LIBS) $(APU_LIBS)

mod_api2_la_SOURCES = $(SOURCE_FILES2)
mod_api2_la_LDFLAGS = -module -avoid-version -no-undefined
mod_api2_la_LIBADD  = libapi_common.a $(libdir)/libcommon_core.la $(APR_LIBS) $(APU_LIBS)

include ../install-pkglib.mk
