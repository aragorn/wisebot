# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_tcp.la
mod_tcp_la_SOURCES = mod_tcp.c
mod_tcp_la_LDFLAGS = -module -avoid-version -no-undefined
mod_tcp_la_LIBADD  = $(libdir)/libcommon_core.la \
                     $(libdir)/libcommon_util.la \
                     $(libdir)/softbot/mod_api.la

include ../install-pkglib.mk
