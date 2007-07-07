# $Id$

include $(top_builddir)/Make.defs

MAINTAINERCLEANFILES = Makefile Makefile.in

pkglib_LTLIBRARIES   = mod_bigram.la \
                       mod_rmas.la mod_rmac2.la mod_http_rmac.la \
                       mod_koma.la \
                       mod_dummy_ma.la \
                       mod_kor2chn_translator.la \
                       mod_daum_koma.la \
                       mod_daum_koma_exact.la \
                       mod_pkoma.la

lib_LTLIBRARIES      = liboldcodetrans.la

check_PROGRAMS       = test_mod_koma test_mod_pkoma

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
mod_rmac2_la_LIBADD  = $(APR_LIBS) $(APU_LIBS)
mod_rmac2_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                       $(APR_INCLUDES) $(APU_INCLUDES)
mod_rmac2_la_LDFLAGS = -module -avoid-version -no-undefined \
						$(APR_LDFLAGS) $(APU_LDFLAGS)

mod_http_rmac_la_SOURCES = mod_http_rmac.c
mod_http_rmac_la_LIBADD  = $(APR_LIBS) $(APU_LIBS)
mod_http_rmac_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                           $(APR_INCLUDES) $(APU_INCLUDES)
mod_http_rmac_la_LDFLAGS = -module -avoid-version

mod_koma_la_SOURCES = mod_koma.c mod_koma.h \
                      mod_koma_complex_noun_support.c \
                      mod_koma_complex_noun_support.h
mod_koma_la_LDFLAGS = -module -avoid-version -L$(libdir) -lkoma -lkomatagger

test_mod_koma_SOURCES = test_mod_koma.c mod_koma.c
test_mod_koma_CFLAGS  = -DTEST_PROGRAM
test_mod_koma_LDFLAGS = -L$(libdir) -lkoma -lkomatagger
test_mod_koma_LDADD   = $(libdir)/libcommon_core.la \
						$(libdir)/libcommon_util.la \
						$(pkglibdir)/mod_api2.la

if USE_DAUM_KOMA
mod_daum_koma_la_SOURCES = mod_daum_koma.c
mod_daum_koma_la_LDFLAGS = -module -avoid-version -L$(libdir) -lhanl

mod_daum_koma_exact_la_SOURCES = mod_daum_exact.c
mod_daum_koma_exact_la_LDFLAGS = -module -avoid-version -L$(libdir) -lhanl
else
mod_daum_koma_la_SOURCES = 
mod_daum_koma_la_LDFLAGS = -module -avoid-version
mod_daum_koma_exact_la_SOURCES = 
mod_daum_koma_exact_la_LDFLAGS = -module -avoid-version
endif

if USE_POSTECH_KOMA
mod_pkoma_la_SOURCES = mod_pkoma.c mod_pkoma.h 
mod_pkoma_la_LDFLAGS = -module -avoid-version -L$(libdir) -lpkoma

test_mod_pkoma_SOURCES = test_mod_pkoma.c mod_pkoma.c
test_mod_pkoma_CFLAGS  = -DTEST_PROGRAM
test_mod_pkoma_LDFLAGS = -L$(libdir) -lpkoma
test_mod_pkoma_LDADD   = $(libdir)/libcommon_core.la \
						 $(libdir)/libcommon_util.la
else
mod_pkoma_la_SOURCES = 
mod_pkoma_la_LDFLAGS = -module -avoid-version
test_mod_pkoma_SOURCES = 
endif

include ../install-pkglib.mk

