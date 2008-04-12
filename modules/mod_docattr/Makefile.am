# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_docattr2.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = mod_docattr.la
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_docattr2.la
check_LTLIBRARIES = mod_docattr.la
endif

mod_docattr_la_SOURCES = mod_docattr.c mod_docattr.h
mod_docattr_la_LDFLAGS = $(MODULES_LDFLAGS)

mod_docattr2_la_SOURCES = mod_docattr2.c
mod_docattr2_la_LDFLAGS = $(MODULES_LDFLAGS)

## pkglib_LTLIBRARIES = mod_docattr_new.la
## mod_docattr_new_la_SOURCES = mod_docattr_new.c mod_docattr_new.h
## mod_docattr_new_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
