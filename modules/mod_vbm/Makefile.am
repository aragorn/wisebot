# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_vbm.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_vbm.la
endif

mod_vbm_la_SOURCES = mod_vbm.c  mod_vbm.h
mod_vbm_la_LDFLAGS = $(MODULES_LDFLAGS)

include ../install-pkglib.mk
