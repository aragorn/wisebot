# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_daemon_replication.la mod_daemon_replication2.la

mod_daemon_replication_la_SOURCES = mod_daemon_replication.c mod_daemon_replication.h
mod_daemon_replication_la_LDFLAGS = -module -avoid-version -no-undefined

mod_daemon_replication2_la_SOURCES = mod_daemon_replication2.c mod_daemon_replication.h
mod_daemon_replication2_la_LDFLAGS = -module -avoid-version -no-undefined

include ../install-pkglib.mk
