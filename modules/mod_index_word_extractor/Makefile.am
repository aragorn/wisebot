# $Id$

include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES   = mod_bigram.la \
                       mod_rmas.la mod_rmac2.la \
                       mod_koma.la \
                       mod_dummy_ma.la \
                       mod_kor2chn_translator.la
if USE_DAUM_KOMA
pkglib_LTLIBRARIES  += mod_daum_koma.la
endif

lib_LTLIBRARIES      = liboldcodetrans.la

liboldcodetrans_la_SOURCES = \
                        lib/ma_chn.c lib/ma_chn.h \
                        lib/ma_code.c lib/ma_code.h \
                        lib/ma_kssm.c lib/ma_kssm.h \
                        lib/ma_jamo.h \
                        lib/old_softbot.h
if AIX5
liboldcodetrans_la_LDFLAGS = -avoid-version -static
else
liboldcodetrans_la_LDFLAGS = -avoid-version -shared
endif

mod_bigram_la_SOURCES = mod_bigram.c mod_bigram.h \
                        mod_bigram_truncation_search_support.c \
                        mod_bigram_truncation_search_support.h
mod_bigram_la_LDFLAGS = -module -avoid-version
mod_bigram_la_LIBADD  = liboldcodetrans.la

mod_rmas_la_SOURCES   = mod_rmas.c
mod_rmas_la_LDFLAGS   = -module -avoid-version

mod_dummy_ma_la_SOURCES = mod_dummy_ma.c
mod_dummy_ma_la_LDFLAGS = -module -avoid-version
mod_dummy_ma_la_LIBADD  = liboldcodetrans.la

mod_kor2chn_translator_la_SOURCES = mod_kor2chn_translator.c
mod_kor2chn_translator_la_LDFLAGS = -module -avoid-version
mod_kor2chn_translator_la_LIBADD  = liboldcodetrans.la

mod_rmac2_la_SOURCES = mod_rmac2.c
mod_rmac2_la_LDFLAGS = -module -avoid-version

mod_koma_la_SOURCES = mod_koma.c mod_koma.h \
                      mod_koma_complex_noun_support.c \
                      mod_koma_complex_noun_support.h
mod_koma_la_LDFLAGS = -module -avoid-version -L$(libdir) -lkoma -lkomatagger

if USE_DAUM_KOMA
mod_daum_koma_la_SOURCES = mod_daum_koma.c
mod_daum_koma_la_LDFLAGS = -module -avoid-version -L$(libdir) -lhanl
endif

include ../install-pkglib.mk

