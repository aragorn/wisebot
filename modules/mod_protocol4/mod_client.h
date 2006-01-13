/*
 * $Id$
 */
#ifndef _SOFTBOT_CLIENT_H_
#define _SOFTBOT_CLIENT_H_ 1

#include "softbot.h"

#include <sys/time.h>

int sb4_com_help(char *result);
int sb4_com_rmas_run(int sockfd, char *arg);
int sb4_com_index_word_extractor(int sockfd, char *arg);
int sb4_com_tokenizer (int sockfd, char *arg);
int sb4_com_qpp (int sockfd, char *arg, void* word_db);
int sb4_com_get_doc_size(int sockfd, char *arg);
int sb4_com_get_abstracted_doc(int sockfd, char *arg);
int sb4_com_get_field(int sockfd, char *arg);
int sb4_com_undelete (int sockfd, char *arg);
int sb4_com_get_wordid (int sockfd, char *arg, void* word_db);
int sb4_com_get_new_wordid(int sockfd, char *arg, void* word_db);
int sb4_com_get_docid(int sockfd, char *arg, void* did_db);
int sb4_com_index_list(int sockfd, char *arg, char *field, int count);
int sb4_com_get_word_by_wordid (int sockfd, char *arg, void* word_db);
int sb4_com_del_system_doc (int sockfd, char *arg);
int sb4_com_doc_count(int sockfd);
int sb4_com_get_oid_field(int sockfd, char *arg, void* did_db); 
int sb4_com_index_word_extractor2(int sockfd, char *arg);
static int get_str_item(char *dest, char *dit, char *key, char delimiter, int len);

#endif
