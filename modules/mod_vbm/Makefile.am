# $Id$
include $(top_builddir)/Make.defs 
MAINTAINERCLEANFILES = Makefile Makefile.in

##AM_CFLAGS = `glib-config --cflags`

libexec_LTLIBRARIES = mod_vbm.la
mod_vbm_la_SOURCES = mod_vbm.c  mod_vbm.h
mod_vbm_la_LDFLAGS = -module -avoid-version

include ../install-libexec.mk
