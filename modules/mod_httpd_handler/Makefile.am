# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_standard_handler.la \
                     mod_search_sbhandler.la \
                     mod_agent_sbhandler.la

mod_standard_handler_la_LDFLAGS = -module -avoid-version -no-undefined
mod_standard_handler_la_LIBADD  = $(libdir)/libcommon_core.la \
                                  $(libdir)/libcommon_util.la \
                                  $(pkglibdir)/mod_api.la \
                                  $(APR_LIBS) $(APU_LIBS) 
mod_standard_handler_la_CFLAGS  = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
mod_standard_handler_la_SOURCES = mod_standard_handler.c mod_standard_handler.h \
                                  common_sbhandler.c \
                                  document_sbhandler.c \
                                  handler_util.c handler_util.h

mod_search_sbhandler_la_LDFLAGS = -module -avoid-version -no-undefined
mod_search_sbhandler_la_LIBADD  = $(libdir)/libcommon_core.la \
                                  $(libdir)/libcommon_util.la \
                                  $(pkglibdir)/mod_api.la \
                                  $(APR_LIBS) $(APU_LIBS) 
mod_search_sbhandler_la_CFLAGS  = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
mod_search_sbhandler_la_SOURCES = mod_search_sbhandler.c \
                                  handler_util.c handler_util.h \
                                  mod_standard_handler.h 

mod_agent_sbhandler_la_LDFLAGS = -module -avoid-version -no-undefined
mod_agent_sbhandler_la_LIBADD  = $(libdir)/libcommon_core.la \
                                 $(libdir)/libcommon_util.la \
                                 $(pkglibdir)/mod_api.la \
                                 $(APR_LIBS) $(APU_LIBS) 
mod_agent_sbhandler_la_CFLAGS  = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
mod_agent_sbhandler_la_SOURCES = mod_agent_sbhandler.c \
                                 handler_util.c handler_util.h \
                                 mod_agent_handler.h 
include ../install-pkglib.mk
