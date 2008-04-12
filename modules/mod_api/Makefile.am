# $Id$
if CYGWIN
noinst_LTLIBRARIES = libapi_common.la mod_api.la mod_api2.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = libapi_common.la
pkglib_LTLIBRARIES = mod_api.la mod_api2.la
endif

libapi_common_la_SOURCES = \
				mod_api.c \
				lexicon.c lexicon.h \
				cdm.c cdm.h \
				cdm2.c cdm2.h \
				did.c did.h \
				vbm.c vbm.h \
				tcp.c tcp.h \
				udp.c udp.h \
				qpp.c qpp.h \
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
libapi_common_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                           $(APR_INCLUDES) $(APU_INCLUDES)
libapi_common_la_LDFLAGS = -shared -avoid-version
libapi_common_la_LIBADD  = $(APR_LIBS) $(APU_LIBS)

SOURCE_FILES1 = qp.c qp.h \
				docattr.c docattr.h

SOURCE_FILES2 = qp2.c qp2.h \
				docattr2.c docattr2.h

mod_api_la_SOURCES = $(SOURCE_FILES1)
mod_api_la_LDFLAGS = $(MODULES_LDFLAGS)
mod_api_la_LIBADD  = libapi_common.la

mod_api2_la_SOURCES = $(SOURCE_FILES2)
mod_api2_la_LDFLAGS = $(MODULES_LDFLAGS)
mod_api2_la_LIBADD  = libapi_common.la

