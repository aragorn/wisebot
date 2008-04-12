# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_vrm.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_vrm.la
endif

mod_vrm_la_SOURCES = vrm.h mod_vrm.c mod_vrm.h
mod_vrm_la_LDFLAGS = $(MODULES_LDFLAGS)

include ../install-pkglib.mk
