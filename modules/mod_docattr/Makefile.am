# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_docattr.la mod_docattr2.la

mod_docattr_la_SOURCES = mod_docattr.c mod_docattr.h
mod_docattr_la_LDFLAGS = -module -avoid-version -no-undefined
mod_docattr_la_LIBADD  = $(libdir)/libcommon_core.la \
                         $(pkglibdir)/mod_api.la

mod_docattr2_la_SOURCES = mod_docattr2.c
mod_docattr2_la_LDFLAGS = -module -avoid-version
mod_docattr2_la_LIBADD  = $(libdir)/libcommon_core.la \
                         $(libdir)/$(PACKAGE)/mod_api.la
## pkglib_LTLIBRARIES = mod_docattr_new.la
## mod_docattr_new_la_SOURCES = mod_docattr_new.c mod_docattr_new.h
## mod_docattr_new_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
