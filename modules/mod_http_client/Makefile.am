# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_http_client.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = test_http_client.la
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_http_client.la
check_LTLIBRARIES = test_http_client.la
endif

mod_http_client_la_SOURCES = mod_http_client.c mod_http_client.h \
	                        http.c http.h \
	                        http_buffering.c http_buffering.h \
	                        http_client_status.h http_reserved_words.h \
							parsedate.c parsedate.h
mod_http_client_la_LDFLAGS = -module $(MODULES_LDFLAGS)

test_http_client_la_SOURCES = test_http_client.c 
test_http_client_la_LDFLAGS = -module $(MODULES_LDFLAGS)

include ../install-pkglib.mk
