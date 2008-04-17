# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_udp.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_udp.la
endif

mod_udp_la_SOURCES = mod_udp.c mod_udp.h
mod_udp_la_LDFLAGS = -module $(MODULES_LDFLAGS)

include ../install-pkglib.mk
