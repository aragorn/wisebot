# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_http_client.la test_http_client.la
mod_http_client_la_SOURCES = mod_http_client.c mod_http_client.h \
	                        http.c http.h \
	                        http_buffering.c http_buffering.h \
	                        http_client_status.h http_reserved_words.h \
							parsedate.c parsedate.h

mod_http_client_la_LDFLAGS = -module -avoid-version

test_http_client_la_SOURCES = test_http_client.c 
test_http_client_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
