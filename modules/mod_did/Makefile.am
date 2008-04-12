# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_did.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = mod_did2.la
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_did.la
check_LTLIBRARIES = mod_did2.la
endif

mod_did_la_SOURCES = mod_did.c
mod_did_la_LDFLAGS = $(MODULES_LDFLAGS)

mod_did2_la_SOURCES = mod_did2.c
mod_did2_la_LDFLAGS = $(MODULES_LDFLAGS)

#mod_did_daemon_la_SOURCES = mod_did_daemon.c \
#							mod_did_client.h mod_did_daemon.h \
#							mod_did_message.h
#mod_did_daemon_la_LDFLAGS = -module -avoid-version

#mod_did_client_la_SOURCES = mod_did_client.c mod_did_client.h
#mod_did_client_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
