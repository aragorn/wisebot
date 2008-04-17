# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_softbot4.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_softbot4.la
endif

mod_softbot4_la_SOURCES = mod_softbot4.c mod_softbot4.h
mod_softbot4_la_LDFLAGS = -module $(MODULES_LDFLAGS)

include ../install-pkglib.mk
