# $Id$ 
## 적혀 있는 순서대로 들어가서 make가 됨.

SUBDIRS = mod_api \
	mod_mp \
	mod_tcp mod_udp \
	mod_sfs mod_ifs \
	mod_indexer \
	mod_qpp mod_qp \
	mod_tokenizer mod_morpheme mod_index_word_extractor \
	mod_docapi mod_cdm mod_docattr \
	mod_protocol4 mod_softbot4 \
	mod_did \
	mod_xmlparser \
	mod_lexicon \
	mod_site mod_site_general \
	mod_httpd \
	mod_httpd_handler \
	mod_http_client \
	mod_vrm \
	mod_replication \
	#

pkglib_LTLIBRARIES =
if CYGWIN
pkglib_LTLIBRARIES += mod_cygwin.la

mod_cygwin_la_SOURCES = 
mod_cygwin_la_LDFLAGS = -module $(MODULES_LDFLAGS)
mod_cygwin_la_LIBADD  = mod_api/mod_api2.la \
                        mod_mp/mod_mp.la \
                        mod_tcp/mod_tcp.la \
                        mod_sfs/mod_sfs.la mod_ifs/mod_ifs.la \
                        mod_indexer/mod_daemon_indexer2.la \
                        mod_indexer/mod_index_each_doc.la \
                        mod_qpp/mod_qpp.la mod_qp/mod_qp2.la \
                        mod_site_general/mod_site_general2.la \
                        mod_tokenizer/mod_tokenizer.la \
                        mod_morpheme/mod_morpheme.la \
                        mod_index_word_extractor/mod_dummy_ma.la \
                        mod_index_word_extractor/mod_koma.la \
                        mod_index_word_extractor/mod_bigram.la \
                        mod_index_word_extractor/mod_kor2chn_translator.la \
                        mod_index_word_extractor/mod_rmas.la \
                        mod_index_word_extractor/mod_rmac2.la \
                        mod_docapi/mod_docapi.la \
                        mod_cdm/mod_cdm3.la mod_docattr/mod_docattr2.la \
                        mod_did/mod_did.la \
                        mod_xmlparser/mod_simple_xmlparser.la \
                        mod_lexicon/mod_lexicon.la \
                        mod_httpd/mod_httpd.la \
                        mod_httpd_handler/mod_httpd_handler.la \
                        mod_http_client/mod_http_client.la \
                        $(libdir)/libcommon_util.la $(libdir)/libcommon_core.la -lfl
                        #
endif

# mod_qpp
# mod_site_general

# Deprecated modules
#	mod_vbm \
#	mod_frm \
#	mod_test \
#	mod_hash \
#	mod_vrfi \
#	mod_spool \
#	mod_vrf \
#	mod_oldlexicon \
#	mod_document_database \
#	mod_cdmi \
#	mod_filter \
#	mod_cdmi \
#	mod_crawler
#	mod_sbcdm_tcp \
#	mod_cdmclt \
#	mod_charconv \
#

