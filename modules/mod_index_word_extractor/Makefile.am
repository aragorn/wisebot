# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_bigram.la \
                     mod_rmas.la mod_rmac2.la \
                     mod_koma.la \
                     mod_dummy_ma.la \
                     mod_kor2chn_translator.la \
                     mod_daum_koma.la \
                     mod_daum_koma_exact.la \
                     mod_pkoma.la \
                     mod_utf8.la
pkglib_LTLIBRARIES = 
check_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_bigram.la \
                     mod_rmas.la mod_rmac2.la \
                     mod_koma.la \
                     mod_dummy_ma.la \
                     mod_kor2chn_translator.la \
                     mod_daum_koma.la \
                     mod_daum_koma_exact.la \
                     mod_pkoma.la \
                     mod_utf8.la
check_LTLIBRARIES = 
endif


lib_LTLIBRARIES      = liboldcodetrans.la

check_PROGRAMS       = test_mod_koma test_mod_koma_complex_noun test_mod_pkoma

liboldcodetrans_la_SOURCES = \
                        lib/ma_chn.c lib/ma_chn.h \
                        lib/ma_code.c lib/ma_code.h \
                        lib/ma_kssm.c lib/ma_kssm.h \
                        lib/ma_jamo.h \
                        lib/old_softbot.h
if AIX5
liboldcodetrans_la_LDFLAGS = -static -avoid-version
else
liboldcodetrans_la_LDFLAGS = -shared -avoid-version -no-undefined
endif

mod_bigram_la_SOURCES = mod_bigram.c mod_bigram.h \
                        mod_bigram_truncation_search_support.c \
                        mod_bigram_truncation_search_support.h
mod_bigram_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_bigram_la_LIBADD  = liboldcodetrans.la

mod_rmas_la_SOURCES   = mod_rmas.c
mod_rmas_la_LDFLAGS   = -module $(MODULES_LDFLAGS)

mod_dummy_ma_la_SOURCES = mod_dummy_ma.c
mod_dummy_ma_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_dummy_ma_la_LIBADD  = liboldcodetrans.la

mod_utf8_la_SOURCES = mod_utf8.c utf8_analyzer.c
mod_utf8_la_LDFLAGS = -module $(MODULES_LDFLAGS)

mod_kor2chn_translator_la_SOURCES = mod_kor2chn_translator.c
mod_kor2chn_translator_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_kor2chn_translator_la_LIBADD  = liboldcodetrans.la

mod_rmac2_la_SOURCES = mod_rmac2.c
mod_rmac2_la_LIBADD  = $(APR_LIBS) $(APU_LIBS)
mod_rmac2_la_CFLAGS  = $(APR_CPPFLAGS) $(APR_CFLAGS) \
                       $(APR_INCLUDES) $(APU_INCLUDES)
mod_rmac2_la_LDFLAGS = -module $(MODULES_LDFLAGS) \
						$(APR_LDFLAGS) $(APU_LDFLAGS)

mod_koma_la_SOURCES = mod_koma.c mod_koma.h \
                      mod_koma_complex_noun_support.c \
                      mod_koma_complex_noun_support.h
mod_koma_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(includedir)
mod_koma_la_LDFLAGS  = -module $(MODULES_LDFLAGS) -L$(libdir) -lkoma -lkomatagger

test_mod_koma_SOURCES = test_mod_koma.c
test_mod_koma_CFLAGS  = -DMOD_KOMA_ENABLED
test_mod_koma_LDFLAGS = -L$(libdir) -lkoma -lkomatagger
test_mod_koma_LDADD   = $(libdir)/libcommon_core.la \
						$(libdir)/libcommon_util.la \
						$(pkglibdir)/mod_api2.la \
                        mod_koma.la

test_mod_koma_complex_noun_SOURCES = test_mod_koma_complex_noun.c
test_mod_koma_complex_noun_LDFLAGS = -L$(libdir) -lkoma -lkomatagger
test_mod_koma_complex_noun_LDADD   = $(libdir)/libcommon_core.la \
			                         $(libdir)/libcommon_util.la \
						             $(pkglibdir)/mod_api2.la \
                                     mod_koma.la

if USE_DAUM_KOMA
mod_daum_koma_la_SOURCES = mod_daum_koma.c
mod_daum_koma_la_LDFLAGS = -module $(MODULES_LDFLAGS) -L$(libdir) -lhanl

mod_daum_koma_exact_la_SOURCES = mod_daum_exact.c
mod_daum_koma_exact_la_LDFLAGS = -module $(MODULES_LDFLAGS) -L$(libdir) -lhanl
else
mod_daum_koma_la_SOURCES = 
mod_daum_koma_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_daum_koma_exact_la_SOURCES = 
mod_daum_koma_exact_la_LDFLAGS = -module $(MODULES_LDFLAGS)
endif

if USE_POSTECH_KOMA
mod_pkoma_la_SOURCES = mod_pkoma.c mod_pkoma.h 
mod_pkoma_la_LDFLAGS = -module -avoid-version -L$(libdir) -lpkoma

test_mod_pkoma_SOURCES = test_mod_pkoma.c
test_mod_pkoma_CFLAGS  = -DMOD_PKOMA_ENABLED
test_mod_pkoma_LDFLAGS = -L$(libdir) -lpkoma
test_mod_pkoma_LDADD   = $(libdir)/libcommon_core.la \
						 $(libdir)/libcommon_util.la \
						 mod_pkoma.la
else
mod_pkoma_la_SOURCES = 
mod_pkoma_la_LDFLAGS = -module $(MODULES_LDFLAGS)
test_mod_pkoma_SOURCES = test_mod_pkoma.c
test_mod_pkoma_CFLAGS  = 
endif

include ../install-pkglib.mk

