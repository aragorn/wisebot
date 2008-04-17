# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_daemon_replication.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_daemon_replication.la
endif

mod_daemon_replication_la_SOURCES = mod_daemon_replication.c \
                                    mod_daemon_replication.h
mod_daemon_replication_la_LDFLAGS = -module $(MODULES_LDFLAGS)

include ../install-pkglib.mk
