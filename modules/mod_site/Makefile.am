# $Id$
include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

libexec_LTLIBRARIES = mod_supreme_court_precedent.la \
					  mod_supreme_court_literature.la \
					  mod_supreme_court_regulation.la \
					  mod_supreme_court_statute.la \
					  mod_supreme_court_legalj.la \
					  mod_lgcaltex.la \
					  mod_supreme_court.la \
					  mod_momaf.la \
					  mod_supreme_home.la

mod_supreme_court_la_SOURCES = $(mod_supreme_court_precedent_la_SOURCES) \
		$(mod_supreme_court_literature_la_SOURCES) \
		$(mod_supreme_court_regulation_la_SOURCES) \
		$(mod_supreme_court_statute_la_SOURCES) \
		$(mod_supreme_court_legalj_la_SOURCES)
mod_supreme_court_la_LDFLAGS = -module -avoid-version

mod_supreme_court_precedent_la_SOURCES = mod_docattr_supreme_precedent.c \
									     mod_docattr_supreme_precedent.h \
										 mod_qp_precedent.c
mod_supreme_court_precedent_la_LDFLAGS = -module -avoid-version

mod_supreme_court_literature_la_SOURCES = mod_docattr_supreme_literature.c \
								 	      mod_docattr_supreme_literature.h \
										  mod_qp_literature.c
mod_supreme_court_literature_la_LDFLAGS = -module -avoid-version

mod_supreme_court_regulation_la_SOURCES = mod_docattr_supreme_regulation.c \
									      mod_docattr_supreme_regulation.h \
										  mod_qp_regulation.c
mod_supreme_court_regulation_la_LDFLAGS = -module -avoid-version

mod_supreme_court_statute_la_SOURCES = mod_docattr_supreme_statute.c \
									   mod_docattr_supreme_statute.h \
									   mod_qp_statute.c
mod_supreme_court_statute_la_LDFLAGS = -module -avoid-version

mod_supreme_court_legalj_la_SOURCES = mod_docattr_supreme_legalj.c \
									  mod_docattr_supreme_legalj.h \
									  mod_qp_legalj.c
mod_supreme_court_legalj_la_LDFLAGS = -module -avoid-version

mod_lgcaltex_la_SOURCES = mod_docattr_lgcaltex.c \
                          mod_docattr_lgcaltex.h \
                          mod_qp_lgcaltex.c
mod_lgcaltex_la_LDFLAGS = -module -avoid-version 

mod_momaf_la_SOURCES = mod_docattr_momaf.c \
                       mod_docattr_momaf.h \
                       mod_qp_momaf.c
mod_momaf_la_LDFLAGS = -module -avoid-version 

mod_supreme_home_la_SOURCES = mod_docattr_supreme_home.c \
                              mod_docattr_supreme_home.h \
							  mod_qp_supreme_home.c
mod_supreme_home_la_LDFLAGS = -module -avoid-version

include ../install-libexec.mk
