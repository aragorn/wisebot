# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_tcp.la
mod_tcp_la_SOURCES = mod_tcp.c
mod_tcp_la_LDFLAGS = -module -avoid-version

include ../install-libexec.mk

