# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_protocol4.la 

mod_protocol4_la_SOURCES = mod_protocol4.c mod_protocol4.h mod_client.c mod_client.h
mod_protocol4_la_LDFLAGS = -module -avoid-version -no-undefined
mod_protocol4_la_LIBADD  = $(libdir)/libcommon_core.la

				
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
