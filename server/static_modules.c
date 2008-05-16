/* $Id$ */

#include "common_core.h"
#include "modules.h"

extern module server_module;
#ifdef CYGWIN
extern module mp_module;
extern module api_module;
extern module tcp_module;
extern module simple_xmlparser_module;
extern module cdm3_module;
extern module did_module;

extern module docapi_module;
extern module docattr2_module;
extern module docattr_general2_module;
extern module lexicon_module;

extern module httpd_module;
extern module http_core_module;
extern module core_httpd_module;
extern module httpd_connection_module;
extern module standard_handler_module;
extern module search_sbhandler_module;
extern module http_client_module;

extern module index_each_doc_module;
extern module daemon_indexer2_module;

extern module tokenizer_module;
extern module dummy_ma_module;
extern module kor2chn_translator_module;
extern module koma_module;
extern module koma_complex_noun_support_module;
extern module bigram_module;
extern module bigram_truncation_search_support_module;
extern module rmac2_module;
extern module sfs_module;
extern module ifs_module;
extern module rmas_module;
extern module morpheme_module;

extern module qpp_module;
extern module qp2_module;
extern module qp_general2_module;
#endif

module *server_static_modules[] = {
	&server_module,
#ifdef CYGWIN
	&mp_module,
	&api_module,
	&tcp_module,
	&simple_xmlparser_module,
	&cdm3_module,
	&did_module,

	&docapi_module,
	&docattr2_module,
	&docattr_general2_module,
	&lexicon_module,

	&httpd_module,
	&http_core_module,
	&core_httpd_module,
	&httpd_connection_module,
	&standard_handler_module,
	&search_sbhandler_module,
	&http_client_module,

	&index_each_doc_module,
	&daemon_indexer2_module,

	&tokenizer_module,
	&dummy_ma_module,
	&kor2chn_translator_module,
	&koma_module,
	&koma_complex_noun_support_module,
	&bigram_module,
	&bigram_truncation_search_support_module,
	&rmac2_module,

	&sfs_module,
	&ifs_module,
	&rmas_module,
	&morpheme_module,

	&qpp_module,
	&qp2_module,
	&qp_general2_module,
#endif
	NULL
};

