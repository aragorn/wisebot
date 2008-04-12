# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_standard_handler.la \
                     mod_search_sbhandler.la \
                     mod_agent_sbhandler.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_standard_handler.la \
                     mod_search_sbhandler.la \
                     mod_agent_sbhandler.la
endif

mod_standard_handler_la_LDFLAGS = $(MODULES_LDFLAGS)
mod_standard_handler_la_LIBADD  = $(APR_LIBS) $(APU_LIBS) 
mod_standard_handler_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                                  $(APR_INCLUDES) $(APU_INCLUDES)
mod_standard_handler_la_SOURCES = mod_standard_handler.c \
                                  mod_standard_handler.h \
                                  common_sbhandler.c \
                                  document_sbhandler.c \
                                  index_sbhandler.c \
                                  replication_sbhandler.c \
                                  handler_util.c handler_util.h

mod_search_sbhandler_la_LDFLAGS = $(MODULES_LDFLAGS)
mod_search_sbhandler_la_LIBADD  = $(APR_LIBS) $(APU_LIBS) 
mod_search_sbhandler_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                                  $(APR_INCLUDES) $(APU_INCLUDES)
mod_search_sbhandler_la_SOURCES = mod_search_sbhandler.c \
                                  handler_util.c handler_util.h \
                                  mod_standard_handler.h 

mod_agent_sbhandler_la_LDFLAGS = $(MODULES_LDFLAGS)
mod_agent_sbhandler_la_LIBADD  = $(APR_LIBS) $(APU_LIBS) 
mod_agent_sbhandler_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                                 $(APR_INCLUDES) $(APU_INCLUDES)
mod_agent_sbhandler_la_SOURCES = mod_agent_sbhandler.c \
                                 handler_util.c handler_util.h

include ../install-pkglib.mk
