# $Id$ 
if CYGWIN
pkglib_LTLIBRARIES = 
noinst_LTLIBRARIES = mod_qpp.la mod_qpp1.la
check_LTLIBRARIES = 
else
noinst_LTLIBRARIES =            mod_qpp1.la
pkglib_LTLIBRARIES = mod_qpp.la
check_LTLIBRARIES = 
endif

mod_qpp_la_SOURCES = mod_qpp.c daum_qpp.c \
                    tokenizer.c stack.c  relatedTerm.c \
					mod_qpp.h daum_qpp.h \
                    qpp_test.h relatedTerm.h \
					stack.h synonym.h tokenizer.h 
mod_qpp_la_LDFLAGS = -module $(MODULES_LDFLAGS)

check_PROGRAMS = daum_qpp qpp1_test mod_qpp2 test_ly

AM_YFLAGS = -d -v
mod_qpp1_la_SOURCES = mod_qpp1.c mod_qpp1.h qpp1_yacc.y qpp1_lex.l
mod_qpp1_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_qpp1_la_LIBADD  = $(LEXLIB)

qpp1_test_SOURCES = qpp1_test.c
qpp1_test_LDADD   = mod_qpp1.la $(libdir)/libcommon_core.la $(LEXLIB) 

daum_qpp_SOURCES = daum_qpp.c daum_qpp.h
daum_qpp_CFLAGS = -DTEST_PROGRAM

mod_qpp2_SOURCES = qpp2_yacc.y qpp2_lex.l
mod_qpp2_LDADD   = $(libdir)/libcommon_core.la $(LEXLIB)

test_ly_SOURCES = test_yacc.y test_lex.l
test_ly_LDADD   = $(libdir)/libcommon_core.la $(LEXLIB)

# check_PROGRAMS = qpp_test tokenizer_test stack_test
# 
# qpp_test_SOURCES = qpp_test.c \
# 				   $(top_builddir)/server/log_error.c
# qpp_test_LDADD = $(libdir)/libmod_qpp.la -lau-c-unit 
# qpp_test_LDFLAGS = `glib-config --libs` 
# 
# tokenizer_test_SOURCES = tokenizer_test.c tokenizer.c \
# 				   $(top_builddir)/server/log_error.c
# tokenizer_test_LDADD = $(libdir)/libmod_morpheme.la -lau-c-unit
# tokenizer_test_LDFLAGS = `glib-config --libs`
# 
# stack_test_SOURCES = stack_test.c stack.c \
# 				   $(top_builddir)/server/log_error.c
# stack_test_LDADD = -lau-c-unit
# stack_test_LDFLAGS = `glib-config --libs`
# 
# TESTS = tokenizer_test stack_test qpp_test

include ../install-pkglib.mk
