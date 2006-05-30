# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_cdm.la mod_cdm2.la mod_cdm3.la

mod_cdm_la_SOURCES = mod_cdm.c indexHandle.c \
					 cannedDocServer.h mod_cdm.h 
#					 syncManager.c cannedDocServer.h mod_cdm.h 
mod_cdm_la_LDFLAGS = -module -avoid-version -no-undefined
mod_cdm_la_LIBADD  = $(libdir)/libcommon_core.la \
                     $(pkglibdir)/mod_api.la

mod_cdm2_la_SOURCES = mod_cdm2.c indexHandle.c cannedDocServer.h \
                      mod_cdm2_lock.h \
                      mod_cdm2_file.c mod_cdm2_file.h \
                      mod_cdm2_docattr.c mod_cdm2_docattr.h \
					  mod_cdm2_util.c mod_cdm2_util.h
mod_cdm2_la_LDFLAGS = -module -avoid-version -no-undefined
mod_cdm2_la_LIBADD  = $(libdir)/libcommon_core.la \
                      $(libdir)/libcommon_util.la \
                      $(libdir)/$(PACKAGE)/mod_api.la

mod_cdm3_la_SOURCES = mod_cdm3.c mod_cdm3.h
mod_cdm3_la_LDFLAGS = -module -avoid-version -no-undefined
mod_cdm3_la_LIBADD  = $(libdir)/libcommon_core.la \
                      $(libdir)/libcommon_util.la \
                      $(libdir)/$(PACKAGE)/mod_api.la
include ../install-pkglib.mk
