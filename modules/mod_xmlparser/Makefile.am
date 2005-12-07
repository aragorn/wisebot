# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_xmlparser.la
mod_xmlparser_la_SOURCES = mod_xmlparser.c parser.c \
						   dh.c stack.c checksum.c \
						   checksum.h dh.h parser.h stack.h

if SRCLIB_ICONV
LIBICONV_LA = $(libdir)/libiconv.la
else
if USR_LOCAL_LIB_ICONV
LIBICONV_LA = /usr/local/lib/libiconv.la
else
LIBICONV_LA =
endif
endif

if SRCLIB_EXPAT
LIBEXPAT_LA = $(libdir)/libexpat.la
else
LIBEXPAT_LA =
endif

mod_xmlparser_la_LIBADD = $(LIBEXPAT_LA) $(LIBICONV_LA)
mod_xmlparser_la_LDFLAGS =-module -avoid-version
mod_xmlparser_la_CFLAGS = $(EXPAT_INCLUDES) -fno-strict-aliasing

check_PROGRAMS = teststree testparser
#check_PROGRAMS = testdh teststack testparser teststree
#testdh_SOURCES = testdh.c dh.c
#teststack_SOURCES = teststack.c stack.c

testparser_SOURCES = testparser.c parser.c dh.c stack.c checksum.c
if SOLARIS
testparser_LDADD = $(libdir)/libexpat.la \
                   $(libdir)/libiconv.la \
                   $(libdir)/libsoftbot.la
else
if CYGWIN
testparser_LDADD = $(libdir)/libsoftbot.la
else
testparser_LDADD = $(libdir)/libexpat.la \
                   $(libdir)/libsoftbot.la
endif
endif
testparser_LDFLAGS = -L../../lib -export-dynamic $(DYN_LIB) $(THREAD_LIB)

teststree_SOURCES = teststree.c simpletree.c
teststree_LDADD   = $(libdir)/libsoftbot.la

include ../install-pkglib.mk
