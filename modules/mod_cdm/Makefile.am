# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_cdm.la
mod_cdm_la_SOURCES = mod_cdm.c indexHandle.c \
					 cannedDocServer.h mod_cdm.h 
#					 syncManager.c cannedDocServer.h mod_cdm.h 
mod_cdm_la_LDFLAGS = -module -avoid-version -no-undefined
mod_cdm_la_LIBADD  = $(libdir)/libcommon_core.la \
                     $(libdir)/$(PACKAGE)/mod_api.la

include ../install-pkglib.mk
