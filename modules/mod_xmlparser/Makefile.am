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
testparser_CFLAGS  = -fno-strict-aliasing
testparser_LDADD   = $(libdir)/libcommon_core.la \
                     $(LIBEXPAT_LA) $(LIBICONV_LA)
testparser_LDFLAGS = -export-dynamic $(DYN_LIB) $(THREAD_LIB)

teststree_SOURCES  = teststree.c simpletree.c
teststree_CFLAGS  = -fno-strict-aliasing
teststree_LDADD    = $(libdir)/libcommon_core.la

include ../install-pkglib.mk
