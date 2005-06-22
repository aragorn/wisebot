# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_cdm.la
mod_cdm_la_SOURCES = mod_cdm.c indexHandle.c \
					 cannedDocServer.h mod_cdm.h 
#					 syncManager.c cannedDocServer.h mod_cdm.h 
mod_cdm_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
