# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_did.la mod_did_daemon.la mod_did_client.la

mod_did_daemon_la_SOURCES = mod_did_wrap_daemon.c
mod_did_daemon_la_LDFLAGS = -module -avoid-version

mod_did_client_la_SOURCES = mod_did_wrap_client.c
mod_did_client_la_LDFLAGS = -module -avoid-version

mod_did_la_SOURCES = mod_did.c mod_did.h
mod_did_la_LDFLAGS = -module -avoid-version

#mod_did_daemon_la_SOURCES = mod_did_daemon.c \
#							mod_did_client.h mod_did_daemon.h \
#							mod_did_message.h
#mod_did_daemon_la_LDFLAGS = -module -avoid-version

#mod_did_client_la_SOURCES = mod_did_client.c mod_did_client.h
#mod_did_client_la_LDFLAGS = -module -avoid-version

include ../install-libexec.mk
