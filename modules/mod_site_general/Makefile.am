# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_site_general2.la \
					 mod_test_docattr.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = mod_site_general.la 
else
noinst_LTLIBRARIES = mod_test_docattr.la
pkglib_LTLIBRARIES = mod_site_general2.la
check_LTLIBRARIES = mod_site_general.la 
endif

AM_YFLAGS = -d
BUILT_SOURCES = mod_qp_general_yacc.h

mod_site_general_la_SOURCES = mod_docattr_general.c \
							  mod_qp_general.c \
							  mod_qp_general_yacc.y \
							  mod_qp_general_lex.l
mod_site_general_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_site_general_la_LIBADD  = $(LEXLIB)

mod_site_general2_la_SOURCES = mod_docattr_general2.c \
							  mod_qp_general2.c \
							  mod_qp_general_yacc2.y \
							  mod_qp_general_lex2.l
mod_site_general2_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_site_general2_la_LIBADD  = $(LEXLIB)

mod_test_docattr_la_SOURCES = mod_docattr_test.c \
                              mod_docattr_test.h \
							  mod_qp_test.c
mod_test_docattr_la_LDFLAGS = -module $(MODULES_LDFLAGS)

include ../install-pkglib.mk
