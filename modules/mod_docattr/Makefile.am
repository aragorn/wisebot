# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_docattr.la
mod_docattr_la_SOURCES = mod_docattr.c mod_docattr.h
mod_docattr_la_LDFLAGS = -module -avoid-version

## pkglib_LTLIBRARIES = mod_docattr_new.la
## mod_docattr_new_la_SOURCES = mod_docattr_new.c mod_docattr_new.h
## mod_docattr_new_la_LDFLAGS = -module -avoid-version

include ../install-libexec.mk
