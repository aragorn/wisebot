# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_frm.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_frm.la
endif

mod_frm_la_SOURCES = mod_frm.c mod_frm.h
mod_frm_la_LDFLAGS = $(MODULES_LDFLAGS)

include ../install-pkglib.mk
