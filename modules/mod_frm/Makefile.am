# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_frm.la 

mod_frm_la_SOURCES = mod_frm.c mod_frm.h
mod_frm_la_LDFLAGS = -module -avoid-version -no-undefined
mod_frm_la_LIBADD  = $(libdir)/libcommon_core.la

include ../install-pkglib.mk
