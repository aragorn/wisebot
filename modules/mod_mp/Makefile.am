#d $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

SOURCE_FILES = mod_mp.c mod_mp.h proc_list.c proc_list.h

lib_LTLIBRARIES  = libmp.la
libmp_la_SOURCES = $(SOURCE_FILES)
if AIX5
# On AIX5, $(THREAD_LIB) option is required.
# But I don't know why. Further investigation would be needed. --aragorn
libmp_la_LDFLAGS = $(THREAD_LIB) -static -avoid-version
else
libmp_la_LDFLAGS = -shared -avoid-version
endif

libexec_LTLIBRARIES = mod_mp.la 

mod_mp_la_SOURCES = $(SOURCE_FILES)
mod_mp_la_LDFLAGS = -module -avoid-version

#libmod_mp_la_SOURCES = $(SOURCE_FILES)
#libmod_mp_la_LDFLAGS = $(THREAD_LIB) -all-static -avoid-version

include ../install-libexec.mk

