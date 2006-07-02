/* $Id$ */
#ifndef COMMANDS_H
#define COMMANDS_H 1

/* the names of functions that actually execute the command */

/* command: general */
int com_connect (char *);
int com_help (char *);
int com_registry (char *);
int com_config (char *);
int com_status (char *);
int com_status_test (char *);
int com_quit (char *);

/* command: rmas/rmac */
int com_rmas_run(char *);
int com_rmac_run(char *);

//static int com_index_word_stat (char *);
/* command: index_word_extractor & morpheme analyzer */
int com_tokenizer (char *arg);
int com_morpheme (char *);
int com_index_word_extractor (char *);

int com_client_memstat (char *);

int com_qpp (char *);
int com_search (char *);
int search (char *, int silence, int* total_cnt);
int com_delete (char *);
int com_undelete (char *);
int com_delete_doc (char *);

/*static int com_register_doc(char *);*/
/*static int com_register_doc_i(char *);*/
int com_register4_doc(char *);
int com_get_doc(char *);
int com_get_doc_size(char *);
int com_get_field(char *);
int com_get_docattr(char *);
int com_set_docattr(char *);
int com_set_docattr_by_oid(char *);
int com_get_abstracted_doc(char *);
int com_get_abstracted_field(char *);
int com_update_field(char *);

int com_get_wordid(char *);
int com_get_word_by_wordid(char *);
int com_get_new_wordid(char *);
int com_sync_word_db(char *);
int com_get_num_of_wordid(char *);

int com_get_docid(char *);
int com_get_new_docid(char *);
int com_last_regi(char *);
int com_repeat(char *);
int com_xrepeat(char *);
int com_search_setting(char *);
int com_query_test(char *arg);
int com_docattr_query_test(char *arg);
int com_forward_index(char *);
int com_log_level(char *);

int com_test_tokenizer(char *);
int com_strcmp(char *);
int com_connectdb(char *);
int com_selectdoc(char *);

int com_rebuild_docattr(char *);
int com_rebuild_rid(char *);

int make_arg(char ***, int *, char *, const char *);
int destroy_arg(char **, int);
int status_arg(char **, int);
int com_benchmark(char *);

extern char mRmacServerAddr[SHORT_STRING_SIZE];
extern char mRmacServerPort[SHORT_STRING_SIZE];

extern char *docattrFields[MAX_FIELD_NUM];

#endif
