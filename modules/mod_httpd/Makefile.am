# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

BUILT_SOURCES = test_char.h
test_char.h: gen_test_char
	./gen_test_char > test_char.h

noinst_PROGRAMS = gen_test_char #test_main
gen_test_char_SOURCES = gen_test_char.c
gen_test_char_CFLAGS = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)

pkglib_LTLIBRARIES = mod_httpd.la
mod_httpd_la_LDFLAGS = -module -avoid-version $(APR_LDFLAGS) $(APU_LDFLAGS)
mod_httpd_la_LIBADD = $(APR_LIBS) $(APU_LIBS) $(libdir)/libcommon_core.la $(libdir)/libcommon_util.la
mod_httpd_la_CFLAGS = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
mod_httpd_la_SOURCES = mod_httpd.c mod_httpd.h \
                       conf.h config.c \
                       connection.c connection.h \
                       core.c core.h \
                       filter.c filter.h \
                       http_config.c http_config.h \
                       http_core.c http_core.h \
                       http_filter.c http_filter.h \
                       http_protocol.c http_protocol.h \
                       http_request.c http_request.h \
                       http_util.c http_util.h \
                       listen.c listen.h \
                       log.c log.h \
                       protocol.c protocol.h \
                       request.c request.h \
                       util_cfgtree.c util_cfgtree.h \
                       util_filter.c util_filter.h \
                       util_string.c util_string.h \
                       util_time.c util_time.h

#mod_httpd_test_handler_la_LDFLAGS = -module -avoid-version $(APR_LDFLAGS) $(APU_LDFLAGS)
#mod_httpd_test_handler_la_LIBADD = $(APR_LIBS) $(APU_LIBS)
#mod_httpd_test_handler_la_CFLAGS = $(APR_CFLAGS) $(APR_INCLUDES) $(APU_INCLUDES)
#mod_httpd_test_handler_la_SOURCES = mod_httpd_test_handler.c \
#					mod_httpd_test_handler.h 

#test_main_SOURCES = test_main.c $(mod_httpd_la_SOURCES)
#test_main_LDFLAGS = -rdynamic $(DYN_LIB) $(APR_LIB) $(THREAD_LIB)
#test_main_LDADD = mod_httpd.la \
#					../mod_mp/mod_mp.la \
#					../../server/libsoftbotserver.la \
#					../../platform/$(OS_TYPE)/libplatform.la

include ../install-pkglib.mk
