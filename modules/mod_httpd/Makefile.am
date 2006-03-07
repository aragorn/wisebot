# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

BUILT_SOURCES = test_char.h
test_char.h: gen_test_char
	./gen_test_char > test_char.h

noinst_PROGRAMS = gen_test_char #test_main
gen_test_char_SOURCES = gen_test_char.c
gen_test_char_CFLAGS = $(APR_INCLUDES) $(APR_UTIL_INCLUDES)

libexec_LTLIBRARIES = mod_httpd.la mod_httpd_test_handler.la
mod_httpd_la_LDFLAGS = -module -avoid-version 
mod_httpd_la_LIBADD = $(libdir)/libapr.la $(libdir)/libaprutil.la

mod_httpd_la_CFLAGS = $(APR_INCLUDES) $(APR_UTIL_INCLUDES)
mod_httpd_la_SOURCES = mod_httpd.c core.c \
					log.c config.c listen.c \
					connection.c filter.c \
					request.c protocol.c \
					http_filter.c http_request.c \
					http_protocol.c http_config.c \
					http_core.c http_util.c \
					util_cfgtree.c util_filter.c \
					util_time.c \
					conf.h connection.h core.h \
					filter.h http_config.h http_core.h \
					http_filter.h http_protocol.h http_request.h \
					http_util.h listen.h log.h \
					mod_httpd.h protocol.h request.h \
					util_cfgtree.h util_filter.h util_time.h

mod_httpd_test_handler_la_LDFLAGS = -module -avoid-version 
mod_httpd_test_handler_la_LIBADD = $(libdir)/libapr.la $(libdir)/libaprutil.la
mod_httpd_test_handler_la_SOURCES = mod_httpd_test_handler.c \
					mod_httpd_test_handler.h 

#test_main_SOURCES = test_main.c $(mod_httpd_la_SOURCES)
#test_main_LDFLAGS = -rdynamic $(DYN_LIB) $(APR_LIB) $(THREAD_LIB)
#test_main_LDADD = mod_httpd.la \
#					../mod_mp/mod_mp.la \
#					../../server/libsoftbotserver.la \
#					../../platform/$(OS_TYPE)/libplatform.la
