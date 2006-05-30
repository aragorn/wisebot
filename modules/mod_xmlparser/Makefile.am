# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

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
LIBEXPAT_LA = -lexpat
endif

pkglib_LTLIBRARIES = mod_xmlparser.la
mod_xmlparser_la_SOURCES = mod_xmlparser.c parser.c \
						   dh.c stack.c checksum.c \
						   checksum.h dh.h parser.h stack.h
mod_xmlparser_la_LIBADD  = $(libdir)/libcommon_core.la \
                           $(pkglibdir)/mod_api.la \
                           $(LIBEXPAT_LA) $(LIBICONV_LA)
mod_xmlparser_la_LDFLAGS = -module -avoid-version -no-undefined
mod_xmlparser_la_CFLAGS  = -fno-strict-aliasing

check_PROGRAMS = teststree testparser

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
testparser_LDFLAGS = -export-dynamic $(DYN_LIB)

teststree_SOURCES = teststree.c simpletree.c
teststree_LDADD   = $(libdir)/libsoftbot.la
teststree_LDFLAGS = -export-dynamic $(DYN_LIB)

include ../install-pkglib.mk
