# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_vrm.la 

mod_vrm_la_SOURCES = vrm.h mod_vrm.c mod_vrm.h
mod_vrm_la_LDFLAGS = -module -avoid-version

include ../install-libexec.mk
