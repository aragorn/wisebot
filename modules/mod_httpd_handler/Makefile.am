# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_standard_handler.la mod_agent_handler.la

mod_standard_handler_la_LDFLAGS = -module -avoid-version 
mod_standard_handler_la_LIBADD  = $(APR_LIBS) $(APU_LIBS)
mod_standard_handler_la_CFLAGS  = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
mod_standard_handler_la_SOURCES = mod_standard_handler.c \
					              mod_standard_handler.h 

mod_agent_handler_la_LDFLAGS = -module -avoid-version 
mod_agent_handler_la_LIBADD  = $(APR_LIBS) $(APU_LIBS)
mod_agent_handler_la_CFLAGS  = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
mod_agent_handler_la_SOURCES = mod_agent_handler.c \
					           mod_standard_handler.h 

include ../install-pkglib.mk
