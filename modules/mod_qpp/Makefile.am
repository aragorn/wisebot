# $Id$ 
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_qpp.la
mod_qpp_la_SOURCES = mod_qpp.c tokenizer.c stack.c  relatedTerm.c \
					mod_qpp.h qpp_test.h relatedTerm.h \
					stack.h synonym.h tokenizer.h 
mod_qpp_la_LDFLAGS = -module -avoid-version -no-undefined
mod_qpp_la_LIBADD  = $(libdir)/libcommon_core.la \
                     $(pkglibdir)/mod_api.la


# check_PROGRAMS = qpp_test tokenizer_test stack_test
# 
# qpp_test_SOURCES = qpp_test.c \
# 				   $(top_builddir)/server/log_error.c
# qpp_test_LDADD = $(libdir)/libmod_qpp.la -lau-c-unit 
# qpp_test_LDFLAGS = `glib-config --libs` 
# 
# tokenizer_test_SOURCES = tokenizer_test.c tokenizer.c \
# 				   $(top_builddir)/server/log_error.c
# tokenizer_test_LDADD = $(libdir)/libmod_morpheme.la -lau-c-unit
# tokenizer_test_LDFLAGS = `glib-config --libs`
# 
# stack_test_SOURCES = stack_test.c stack.c \
# 				   $(top_builddir)/server/log_error.c
# stack_test_LDADD = -lau-c-unit
# stack_test_LDFLAGS = `glib-config --libs`
# 
# TESTS = tokenizer_test stack_test qpp_test

include ../install-pkglib.mk
