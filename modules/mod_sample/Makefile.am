# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_sample.la mod_sample1.la mod_sample2.la

mod_sample_la_SOURCES = mod_sample.c
mod_sample_la_LDFLAGS = -module -avoid-version

mod_sample1_la_SOURCES = mod_sample1.c
mod_sample1_la_LDFLAGS = -module -avoid-version

mod_sample2_la_SOURCES = mod_sample2.c
mod_sample2_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
