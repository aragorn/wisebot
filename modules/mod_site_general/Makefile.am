# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

AM_YFLAGS = -d
BUILT_SOURCES = mod_qp_general_yacc.h

pkglib_LTLIBRARIES = mod_site_general.la \
					 mod_test_docattr.la

mod_site_general_la_SOURCES = mod_docattr_general.c \
							  mod_qp_general.c \
							  mod_qp_general_yacc.y \
							  mod_qp_general_lex.l
mod_site_general_la_LDFLAGS = -module -avoid-version $(LEXLIB)

mod_test_docattr_la_SOURCES = mod_docattr_test.c \
                              mod_docattr_test.h \
							  mod_qp_test.c
mod_test_docattr_la_LDFLAGS = -module -avoid-version

include ../install-pkglib.mk
