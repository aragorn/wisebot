# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_protocol4.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_protocol4.la
endif

mod_protocol4_la_SOURCES = mod_protocol4.c mod_protocol4.h \
                           mod_client.c mod_client.h
mod_protocol4_la_LDFLAGS = $(MODULES_LDFLAGS)

				
# check_PROGRAMS = mod_protocol4_test
# mod_protocol4_test_SOURCES = mod_protocol4_test.c mod_protocol4.h \
#                       $(top_builddir)/server/log_error.c \
#                       $(top_builddir)/server/hook.c
# mod_protocol4_test_LDFLAGS = `glib-config --libs` -lpthread
# mod_protocol4_test_LDADD = $(libdir)/libmod_qp.a \
#                     $(libdir)/libqpp.a \
#                     $(libdir)/libmorp.a \
#                     $(libdir)/libmod_vrf.a \
#                     $(libdir)/libmod_indexer.a -lau-c-unit
# 
# TESTS = mod_protocol4_test

include ../install-pkglib.mk
