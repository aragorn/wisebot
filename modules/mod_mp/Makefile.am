#d $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

SOURCE_FILES = mod_mp.c mod_mp.h proc_list.c proc_list.h

pkglib_LTLIBRARIES = mod_mp.la 

mod_mp_la_SOURCES = $(SOURCE_FILES)
mod_mp_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk

