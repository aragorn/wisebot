# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_tcp.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_tcp.la
endif

mod_tcp_la_SOURCES = mod_tcp.c
mod_tcp_la_LDFLAGS = $(MODULES_LDFLAGS)

include ../install-pkglib.mk
