# $Id$ 
if CYGWIN
noinst_LTLIBRARIES = mod_qp2.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = mod_qp.la
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_qp2.la
check_LTLIBRARIES = mod_qp.la
endif

mod_qp_la_SOURCES = mod_qp.c mod_qp.h
mod_qp_la_LDFLAGS = $(MODULES_LDFLAGS)
mod_qp_la_LIBADD  = -lm

mod_qp2_la_SOURCES = mod_qp2.c
mod_qp2_la_LDFLAGS = $(MODULES_LDFLAGS)
mod_qp2_la_LIBADD  = -lm

#mod_qp_cdmi_la_SOURCES = mod_qp_cdmi.c mod_qp.h
#mod_qp_cdmi_la_LDFLAGS = -module -avoid-version

#check_PROGRAMS = mod_qp_test
#mod_qp_test_SOURCES = mod_qp_test.c mod_qp.h \
#					  $(top_builddir)/server/log_error.c \
#					  $(top_builddir)/server/hook.c
#mod_qp_test_LDFLAGS = `glib-config --libs` -lpthread
#mod_qp_test_LDADD = $(libdir)/libmod_qp.a \
#					$(libdir)/libqpp.a \
#					$(libdir)/libmorp.a \
#					$(libdir)/libmod_vrf.a \
#					$(libdir)/libmod_indexer.a -lau-c-unit

#TESTS = mod_qp_test

include ../install-pkglib.mk
