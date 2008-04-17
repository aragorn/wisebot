# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_cdm3.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES  = mod_cdm.la mod_cdm2.la
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_cdm3.la
check_LTLIBRARIES  = mod_cdm.la mod_cdm2.la
endif


mod_cdm_la_SOURCES = mod_cdm.c indexHandle.c \
					 cannedDocServer.h mod_cdm.h 
#					 syncManager.c cannedDocServer.h mod_cdm.h 
mod_cdm_la_LDFLAGS = -module $(MODULES_LDFLAGS)

mod_cdm2_la_SOURCES = mod_cdm2.c indexHandle.c cannedDocServer.h \
                      mod_cdm2_lock.h \
                      mod_cdm2_file.c mod_cdm2_file.h \
                      mod_cdm2_docattr.c mod_cdm2_docattr.h \
					  mod_cdm2_util.c mod_cdm2_util.h
mod_cdm2_la_LDFLAGS = -module $(MODULES_LDFLAGS)

mod_cdm3_la_SOURCES = mod_cdm3.c mod_cdm3.h
mod_cdm3_la_LDFLAGS = -module $(MODULES_LDFLAGS)

include ../install-pkglib.mk
