# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_tokenizer.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_tokenizer.la
endif

mod_tokenizer_la_SOURCES = mod_tokenizer.c mod_tokenizer.h
mod_tokenizer_la_LDFLAGS = $(MODULES_LDFLAGS)

include ../install-pkglib.mk
