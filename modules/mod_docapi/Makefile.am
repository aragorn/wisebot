# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_docapi.la #mod_docapi_cdmi.la mod_docapi_test.la
#mod_docapi_la_SOURCES = mod_docapi_libxml.c ## if you want use libxml library
#mod_docapi_la_SOURCES = mod_docapi_expat.c 
mod_docapi_la_SOURCES = mod_docapi_expat_local_temp.c \
						mod_docapi.h
mod_docapi_la_LDFLAGS = -module -avoid-version 

#mod_docapi_cdmi_la_SOURCES = mod_docapi_cdmi.c \
#						mod_docapi.h
#mod_docapi_cdmi_la_LDFLAGS = -module -avoid-version 


#mod_docapi_test_la_SOURCES = testdocapicdmi.c
#mod_docapi_test_la_LDFLAGS = -module -avoid-version 

include ../install-libexec.mk
