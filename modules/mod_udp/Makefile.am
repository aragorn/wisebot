# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_udp.la 
mod_udp_la_SOURCES = mod_udp.c mod_udp.h
mod_udp_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
