# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_http_client.la test_http_client.la
mod_http_client_la_SOURCES = mod_http_client.c mod_http_client.h \
	                        http.c http.h \
	                        http_buffering.c http_buffering.h \
	                        http_client_status.h http_reserved_words.h \
							parsedate.c parsedate.h

mod_http_client_la_LDFLAGS = -module -avoid-version -no-undefined
mod_http_client_la_LIBADD = $(libdir)/libcommon_core.la \
                            $(libdir)/libcommon_util.la \
                            $(pkglibdir)/mod_api.la

test_http_client_la_SOURCES = test_http_client.c 
test_http_client_la_LDFLAGS = -module -avoid-version -no-undefined
test_http_client_la_LIBADD = $(libdir)/libcommon_core.la \
                             $(libdir)/libcommon_util.la \
                             $(pkglibdir)/mod_api.la

include ../install-pkglib.mk
