# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_vrfi.la
mod_vrfi_la_SOURCES = mod_vrfi.c mod_vrfi.h \
					  data_handle.c data_handle.h \
					  header_handle.c header_handle.h
mod_vrfi_la_LDFLAGS = -module -avoid-version -no-undefined
mod_vbm_la_LIBADD  = $(libdir)/libcommon_core.la \
                     $(pkglibdir)/mod_api.la

check_PROGRAMS = mod_vrfi_test
mod_vrfi_test_SOURCES = mod_vrfi.c mod_vrfi.h \
						data_handle.c data_handle.h \
						header_handle.c header_handle.h \
mod_vrfi_test_CFLAGS = -DSTANDALONE_DEBUG -g3
mod_vrfi_test_LDADD = $(libdir)/libcommon_core.la \
                      $(pkglibdir)/mod_api.la

noinst_PROGRAMS = header_reader data_reader master_reader
header_reader_SOURCES = header_reader.c header_handle.h
data_reader_SOURCES = data_reader.c data_handle.h
master_reader_SOURCES = master_reader.c mod_vrfi.h

include ../install-pkglib.mk
