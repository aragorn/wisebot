# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_test_rwlock.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_test_rwlock.la
endif

pkglib_LTLIBRARIES = mod_test_rwlock.la
mod_test_rwlock_la_SOURCES = mod_test_rwlock.c
mod_test_rwlock_la_LDFLAGS = $(MODULES_LDFLAGS)

include ../install-pkglib.mk
