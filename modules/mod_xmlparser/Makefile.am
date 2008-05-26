# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_simple_xmlparser.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = mod_xmlparser.la
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_simple_xmlparser.la
check_LTLIBRARIES = mod_xmlparser.la
endif

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

mod_xmlparser_la_SOURCES = mod_xmlparser.c parser.c \
						   dh.c stack.c checksum.c \
						   checksum.h dh.h parser.h stack.h
mod_xmlparser_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_xmlparser_la_LIBADD  = $(LIBEXPAT_LA) $(LIBICONV_LA)
mod_xmlparser_la_CFLAGS  = -fno-strict-aliasing

mod_simple_xmlparser_la_SOURCES = mod_simple_xmlparser.c
mod_simple_xmlparser_la_LDFLAGS = -module $(MODULES_LDFLAGS)

check_PROGRAMS = teststree testparser testsimpleparser

testparser_SOURCES = testparser.c parser.c dh.c stack.c checksum.c
if SOLARIS
testparser_LDADD = $(LIBEXPAT_LA) \
                   $(LIBICONV_LA) \
                   $(libdir)/libcommon_core.la
else
if CYGWIN
testparser_LDADD = $(libdir)/libcommon_core.la
else
testparser_LDADD = $(LIBEXPAT_LA) \
                   $(libdir)/libcommon_core.la
endif
endif
testparser_LDFLAGS = -export-dynamic $(DYN_LIB)

teststree_SOURCES = teststree.c simpletree.c
teststree_LDADD   = $(libdir)/libcommon_core.la
teststree_LDFLAGS = -export-dynamic $(DYN_LIB)

testsimpleparser_SOURCES = mod_simple_xmlparser.c
testsimpleparser_CFLAGS = -DTEST_PROGRAM

