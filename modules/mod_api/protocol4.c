/* $Id$ */
#include "softbot.h"
#include "mod_api/protocol4.h"

HOOK_STRUCT(
	HOOK_LINK(protocol_open)
	HOOK_LINK(protocol_close)

	HOOK_LINK(sb4c_register_doc)
	HOOK_LINK(sb4c_get_doc)
	HOOK_LINK(sb4c_set_docattr)
	HOOK_LINK(sb4c_delete_doc)
	HOOK_LINK(sb4c_delete_oid)
	HOOK_LINK(sb4c_init_search)
	HOOK_LINK(sb4c_search_doc)
	HOOK_LINK(sb4c_search2_doc)
	HOOK_LINK(sb4c_free_search)
	HOOK_LINK(sb4c_status)
	HOOK_LINK(sb4c_last_docid)
	HOOK_LINK(sb4c_remote_morphological_analyze_doc)

	HOOK_LINK(sb4s_dispatch)
	HOOK_LINK(sb4s_register_doc)
	HOOK_LINK(sb4s_register_doc2)
	HOOK_LINK(sb4s_get_doc)
	HOOK_LINK(sb4s_set_docattr)
	HOOK_LINK(sb4s_delete_doc)
	HOOK_LINK(sb4s_delete_oid)
	HOOK_LINK(sb4s_delete_oid2)
	HOOK_LINK(sb4s_search_doc)
	HOOK_LINK(sb4s_search2_doc)
	HOOK_LINK(sb4s_status)
	HOOK_LINK(sb4s_last_docid)
	HOOK_LINK(sb4s_remote_morphological_analyze_doc)

/* add khyang */	
	HOOK_LINK(sb4s_set_log)
	HOOK_LINK(sb4s_status_str)
	HOOK_LINK(sb4s_help)
	HOOK_LINK(sb4s_rmas)
	HOOK_LINK(sb4s_indexwords)
	HOOK_LINK(sb4s_tokenizer)
	HOOK_LINK(sb4s_qpp)
	HOOK_LINK(sb4s_show_spool)
	HOOK_LINK(sb4s_get_cdm_size)
	HOOK_LINK(sb4s_get_abstract)
	HOOK_LINK(sb4s_get_field)
	HOOK_LINK(sb4s_undel_doc)
	HOOK_LINK(sb4s_config)
	HOOK_LINK(sb4s_get_wordid)
	HOOK_LINK(sb4s_get_new_wordid)
	HOOK_LINK(sb4s_get_docid)	
	HOOK_LINK(sb4s_index_list)
	HOOK_LINK(sb4s_word_list)
	HOOK_LINK(sb4s_del_system_doc)
	HOOK_LINK(sb4s_systemdoc_count)
	HOOK_LINK(sb4s_did_req_comment)
	HOOK_LINK(sb4s_oid_req_comment)
	HOOK_LINK(sb4s_get_oid_field)
)

SB_IMPLEMENT_HOOK_RUN_ALL(int, protocol_open, (), (), SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, protocol_close, (), (), SUCCESS, DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_register_doc, \
	(int sockfd, char *dit, char *body, int body_size), \
	(sockfd, dit, body, body_size),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_get_doc, \
	(int sockfd, uint32_t docid, char *buf, int bufsize), \
	(sockfd, docid, buf, bufsize),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_set_docattr, \
	(int sockfd, char *dit), \
	(sockfd, dit),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_delete_doc,  \
	(int sockfd, uint32_t docid), (sockfd, docid),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_delete_oid, (int sockfd, char *oid), \
	(sockfd, oid),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_init_search, \
	(sb4_search_result_t **result, int list_size), (result, list_size),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_free_search, \
	(sb4_search_result_t **result), (result),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_search_doc, \
	(int sockfd, char *query, char *attr, int listcount, int page, char *sc, \
	 sb4_search_result_t *result), \
	(sockfd, query, attr, listcount, page, sc, result),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_search2_doc, \
	(int sockfd, char *query, char *attr, int listcount, int page, char *sc, \
	 sb4_search_result_t *result), \
	(sockfd, query, attr, listcount, page, sc, result),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_status, \
	(int sockfd, char *cmd, FILE *output), (sockfd, cmd, output), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4c_last_docid, \
	(int sockfd, uint32_t *docid), (sockfd, docid), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,sb4c_remote_morphological_analyze_doc, \
	(int sockfd, char *meta_data , void *send_data , \
	 long send_data_size,  void **receive_data, long *receive_data_size), \
	(sockfd, meta_data, send_data, send_data_size, receive_data, \
	 receive_data_size), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_register_doc, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_register_doc2, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_get_doc, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_set_docattr, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_delete_doc, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_delete_oid, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_delete_oid2, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_search_doc, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_search2_doc, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_dispatch, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_status, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_last_docid, (int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_remote_morphological_analyze_doc,(int sockfd), (sockfd), DECLINE)

/* add khyang */
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_status_str,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_set_log,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_help,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_rmas,(int sockfd), (sockfd), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_indexwords,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_tokenizer,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_qpp,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_show_spool,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_get_cdm_size,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_get_abstract,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_get_field,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_undel_doc,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_config,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_get_wordid,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_get_new_wordid,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_get_docid,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_index_list,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_word_list,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_del_system_doc,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_systemdoc_count,(int sockfd), (sockfd), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_did_req_comment,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_oid_req_comment,(int sockfd), (sockfd), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, sb4s_get_oid_field,(int sockfd), (sockfd), DECLINE)
