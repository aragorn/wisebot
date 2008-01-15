# $Id$
include $(top_builddir)/Make.defs 

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_vbm.la
mod_vbm_la_SOURCES = mod_vbm.c  mod_vbm.h
mod_vbm_la_LDFLAGS = -module -avoid-version -no-undefined
mod_vbm_la_LIBADD  = $(libdir)/libcommon_core.la \
                     $(libdir)/libcommon_util.la \
		             $(libdir)/libapi_common.la

include ../install-pkglib.mk
