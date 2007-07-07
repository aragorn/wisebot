# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_tokenizer.la 
mod_tokenizer_la_SOURCES = mod_tokenizer.c mod_tokenizer.h
mod_tokenizer_la_LDFLAGS = -module -avoid-version -no-undefined
mod_tokenizer_la_LIBADD  = $(libdir)/libcommon_core.la

include ../install-pkglib.mk
