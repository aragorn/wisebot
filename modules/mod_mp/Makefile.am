#d $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_mp.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_mp.la
endif

mod_mp_la_SOURCES = mod_mp.c mod_mp.h proc_list.c proc_list.h
mod_mp_la_LDFLAGS = $(MODULES_LDFLAGS) $(THREAD_LIB)

include ../install-pkglib.mk
