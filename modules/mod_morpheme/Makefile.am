# $Id$
if CYGWIN
noinst_LTLIBRARIES = mod_morpheme.la
pkglib_LTLIBRARIES = 
else
noinst_LTLIBRARIES = 
pkglib_LTLIBRARIES = mod_morpheme.la
endif

mod_morpheme_la_SOURCES = mod_morpheme.c \
				lib/ma.c lib/ma_kor.c lib/ma_eng.c \
				lib/mabi.c lib/mabi_kor.c \
				lib/ma_kssm.c lib/ma_chn.c \
				lib/ma_data.c lib/mak_data.c lib/ma_code.c \
				lib/lb_std.c lib/lb_lex.c \
				mod_morpheme.h 
mod_morpheme_la_LDFLAGS = $(MODULES_LDFLAGS)

#EXTRA_DIST = lib
#
#dist-hook:
#	rm -rf `find $(distdir)/ -name CVS`

#libmod_morpheme_ja_la_SOURCES = jalib/mmap.h csv.h jalib/dictionary_maker.h jalib/viterbi.cpp \
#              jalib/node.h jalib/tokenizer.h jalib/common.h  jalib/darts.h jalib/codeconv.h \
#              jalib/freelist.h jalib/viterbi.h jalib/param.cpp jalib/connector.h \
#              jalib/dictionary.h jalib/japanese_tokenizer.cpp jalib/node_iterator.h \
#              jalib/param.h jalib/csv.cpp jalib/dictionary_maker.cpp jalib/japanese_tokenizer.h \
#              jalib/tagger.cpp jalib/libchasentng.cpp jalib/chasentng.h

#check_PROGRAMS = chasen_test
#chasen_test_SOURCES = chasentng_test.c
#chasen_test_LDADD = $(libdir)/libmod_morpheme_ja.la

#check_PROGRAMS = bigram_test 
#bigram_test_SOURCES = mod_bigram.c bigram_test.c

include ../install-pkglib.mk
