# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_vrm.la 

mod_vrm_la_SOURCES = vrm.h mod_vrm.c mod_vrm.h
mod_vrm_la_LDFLAGS = -module -avoid-version -no-undefined
mod_vrm_la_LIBADD  = $(libdir)/libcommon_core.la

include ../install-pkglib.mk
