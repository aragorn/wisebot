# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES = mod_xmlparser.la
mod_xmlparser_la_SOURCES = mod_xmlparser.c parser.c \
						   dh.c stack.c checksum.c \
						   checksum.h dh.h parser.h stack.h
if SOLARIS
mod_xmlparser_la_LIBADD = $(top_builddir)/lib/libexpat.la /usr/local/lib/libiconv.la
else
mod_xmlparser_la_LIBADD = $(top_builddir)/lib/libexpat.la
endif
mod_xmlparser_la_LDFLAGS =-module -avoid-version
mod_xmlparser_la_CFLAGS = $(EXPAT_INCLUDES) -fno-strict-aliasing

check_PROGRAMS = teststree testparser
#check_PROGRAMS = testdh teststack testparser teststree
#testdh_SOURCES = testdh.c dh.c
#teststack_SOURCES = teststack.c stack.c

testparser_SOURCES = testparser.c parser.c dh.c stack.c checksum.c
if SOLARIS
testparser_LDADD = $(top_builddir)/lib/libexpat.la \
                   $(top_builddir)/lib/libiconv.la \
                   $(top_builddir)/lib/libsoftbotserver.la
else
testparser_LDADD = $(top_builddir)/lib/libexpat.la \
                   $(top_builddir)/lib/libsoftbotserver.la
endif
testparser_LDFLAGS = -L../../lib -export-dynamic -ldl $(THREAD_LIB)

teststree_SOURCES = teststree.c simpletree.c
teststree_LDADD   = $(top_builddir)/lib/libsoftbotserver.la

include ../install-pkglib.mk
