# $Id$
if CYGWIN
pkglib_LTLIBRARIES = mod_httpd_handler.la
else
pkglib_LTLIBRARIES = mod_httpd_handler.la \
                     mod_standard_handler.la \
                     mod_search_sbhandler.la \
                     mod_agent_sbhandler.la
endif

mod_httpd_handler_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_httpd_handler_la_LIBADD  = $(APR_LIBS) $(APU_LIBS) 
mod_httpd_handler_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                               $(APR_INCLUDES) $(APU_INCLUDES)
mod_httpd_handler_la_SOURCES = mod_standard_handler.c \
                               mod_standard_handler.h \
                               mod_search_sbhandler.c \
                               mod_agent_sbhandler.c \
                               common_sbhandler.c \
                               document_sbhandler.c \
                               index_sbhandler.c \
                               replication_sbhandler.c \
                               handler_util.c handler_util.h

if CYGWIN
else
mod_standard_handler_la_LDFLAGS = -module $(MODULES_LDFLAGS)
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

mod_search_sbhandler_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_search_sbhandler_la_LIBADD  = $(APR_LIBS) $(APU_LIBS) 
mod_search_sbhandler_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                                  $(APR_INCLUDES) $(APU_INCLUDES)
mod_search_sbhandler_la_SOURCES = mod_search_sbhandler.c \
                                  handler_util.c handler_util.h \
                                  mod_standard_handler.h 

mod_agent_sbhandler_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_agent_sbhandler_la_LIBADD  = $(APR_LIBS) $(APU_LIBS) 
mod_agent_sbhandler_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                                 $(APR_INCLUDES) $(APU_INCLUDES)
mod_agent_sbhandler_la_SOURCES = mod_agent_sbhandler.c \
                                 handler_util.c handler_util.h
endif
include ../install-pkglib.mk
