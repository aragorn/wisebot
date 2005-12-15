/* $Id$ */
#ifndef _PROTOCOL4_H_
#define _PROTOCOL4_H_ 1

#include "softbot.h"
#include "mod_api/vbm.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SB4_OP_REGISTER_DOC			"103"
#define SB4_OP_REGISTER_DOC2		"107"
#define SB4_OP_GET_DOC				"106"
#define SB4_OP_SET_DOCATTR			"110"
#define SB4_OP_LAST_DOCID			"104"

#define SB4_OP_REGISTER_DOCS		"153"
#define SB4_OP_EXTRACT_FOREIGN_ID	"205"
#define SB4_OP_DELETE_DOC			"402"
#define SB4_OP_DELETE_OID			"404"
#define SB4_OP_DELETE_SYSTEM_DOC		"405"
#define SB4_OP_SEARCH_DOC			"700"
#define SB4_OP_SEARCH2_DOC          "701"
#define SB4_OP_EXTRACT_BODY			"903"
#define SB4_OP_STATUS				"950"
#define SB4_OP_RMA_DOC				"800"


#define SB4_OP_LOG 			"901"  	// log
#define SB4_OP_HELP 			"902"		// help
#define SB4_OP_RMAS 			"903"   	// rmas
#define SB4_OP_INDEXWORDS		"904"    	// indexwords
#define SB4_OP_TOKENIZER 		"905"    	// tokenizer
#define SB4_OP_QPP			"906"    	// qpp
#define SB4_OP_CLIENT_MEMSTAT		"907"    	// client_memstat
#define SB4_OP_DEL_SPOOL_FRONT		"908"	    	// delete_spool_front
#define SB4_OP_SHOW_SPOOL		"909"    	// showspool
#define SB4_OP_GET_DOCATTR		"910"    	// documentdatabase_docattr_get
#define SB4_OP_GET_CDM_SIZE		"911"    	// getsize
#define SB4_OP_GET_ABSTRACT		"912"    	// getabstract
#define SB4_OP_GET_FIELD		"913"    	// getfield
#define SB4_OP_UNDEL_DOC		"914"    	// undelete
#define SB4_OP_SEARCH_DOC_SET 		"915"    	// search_setting and search doc
#define SB4_OP_CONFIG			"916"    	// config
#define SB4_OP_GET_WORDID		"917"    	// get_wordid
#define SB4_OP_GET_NEW_WORDID		"918"    	// get_new_wordid
#define SB4_OP_GET_DOCID		"920"    	// getdocid
#define SB4_OP_STATUS_STR		"921"		// string send recv status
#define SB4_OP_INDEX_LIST		"922"		// index list send
#define SB4_OP_WORD_LIST		"923"		// word list send
#define SB4_OP_SYSTEMDOC_COUNT		"924"   	//system doc count info
#define SB4_OP_DID_REQ_COMMENT		"925"   	//docid get comment
#define SB4_OP_OID_REQ_COMMENT		"926"   	//oid get comment
#define SB4_OP_GET_OID_FIELD		"927"   	//oid get txt

#define SB4_OP_ACK		"ACK"
#define SB4_OP_NAK		"NAK"

#define SB4_TAG_CONT 'C'
#define SB4_TAG_END  'E'
#define SB4_MAX_SEND_SIZE (LONG_LONG_STRING_SIZE)

#define SB4_DEFAULT_NONBLOCKING_IO	(1)

typedef struct {
	char tag;
	char size[7];
} header_t; /* 8 byte */

#define SB4_MAX_DOC_CT	(10)	/* number of category for each document */
#define SB4_MAX_PT_LEN	(4)		/* length of category id in char[] */
#define SB4_MAX_CT_LEN	(8)		/* length of category id in char[] */
#define SB4_MAX_DT_LEN	(12)	/* length of date in char[] */
#define SB4_MAX_DOC_NM	(32)	/* length of document filename */
#define SB4_MAX_OID		(256)	/* length of foreign id */
#define SB4_MAX_DSC_LEN	(4)		/* document security control1 */
#define SB4_MAX_DOC_TT	(100)	/* length of title */
#define SB4_MAX_DOC_AT	(16)	/* length of author */
#define SB4_MAX_DOC_KW	(64)	/* length of keywords */
#define SB4_MAX_DOC_FD	(64)	/* length of field */

typedef struct {
	int HIT; /* hit: relevancy */
	int DID; /* doc: document id */
	int DST; /* delete status: whether document is deleted */
	int DIC; /* document import count: number of category link */
	int DAC; /* document access count: */
	char PT[SB4_MAX_PT_LEN]; /* PROTOCOL */
	char CT[SB4_MAX_DOC_CT * SB4_MAX_CT_LEN]; /* category */
	char DTR[SB4_MAX_DT_LEN];	/* registered date set by softbot automatically */
	char DTC[SB4_MAX_DT_LEN];	/* created date of document set by user arbitrarily */
	char DN[SB4_MAX_DOC_NM];	/* document filename */
	char OID[SB4_MAX_OID];		/* other id, foreign id like url */
	char RID[SB4_MAX_OID];		/* root id, foreign id like url */
	char DSC[SB4_MAX_DSC_LEN];	/* document security control? */
	char TT[SB4_MAX_DOC_TT];	/* title */
	char AT[SB4_MAX_DOC_AT];	/* author */
	char KW[SB4_MAX_DOC_KW];	/* keyword */
	char FD1[SB4_MAX_DOC_FD];	/* field 1 */
	char FD2[SB4_MAX_DOC_FD];	/* field 2 */
	char FD3[SB4_MAX_DOC_FD];	/* field 3 */
	char FD4[SB4_MAX_DOC_FD];	/* field 4 */
	char FD5[SB4_MAX_DOC_FD];	/* field 5 */
	char FD6[SB4_MAX_DOC_FD];	/* field 6 */
	char FD7[SB4_MAX_DOC_FD];	/* field 7 */
	char FD8[SB4_MAX_DOC_FD];	/* field 8 */
	char SUM[SB4_MAX_DOC_TT];	/* summary ? */
} sb4_dit_t;

typedef struct {
	int total_count;
	int received_count;
	int allocated;
	char *word_list;
	char **results;
} sb4_search_result_t;

typedef struct {
	void *data;
	int data_size;
	int allocated_size;
} sb4_merge_buffer_t;

SB_DECLARE_HOOK(int,protocol_open,())
SB_DECLARE_HOOK(int,protocol_close,())

SB_DECLARE_HOOK(int,sb4c_register_doc,(int sockfd, char *dit, char *body, int body_size))
SB_DECLARE_HOOK(int,sb4c_get_doc,(int sockfd, uint32_t docid, char *buf, int bufsize))
SB_DECLARE_HOOK(int,sb4c_set_docattr,(int sockfd, char *dit))
SB_DECLARE_HOOK(int,sb4c_delete_doc,(int sockfd, uint32_t docid))
SB_DECLARE_HOOK(int,sb4c_delete_oid,(int sockfd, char *oid))
SB_DECLARE_HOOK(int,sb4c_init_search,(sb4_search_result_t **result, int list_size))
SB_DECLARE_HOOK(int,sb4c_free_search,(sb4_search_result_t **result))
SB_DECLARE_HOOK(int,sb4c_search_doc,(int sockfd, char *query, char *attr, \
	int listcount, int page, char *sc, sb4_search_result_t *result))
SB_DECLARE_HOOK(int,sb4c_search2_doc,(int sockfd, char *query, char *attr, \
	int listcount, int page, char *sc, sb4_search_result_t *result))
SB_DECLARE_HOOK(int,sb4c_status,(int sockfd, char *cmd, FILE *output))
SB_DECLARE_HOOK(int,sb4c_last_docid,(int sockfd, uint32_t *docid))
SB_DECLARE_HOOK(int,sb4c_remote_morphological_analyze_doc, \
	(int sockfd, char *meta_data , void *send_data , \
	 long send_data_size,  void **receive_data, long *receive_data_size))

SB_DECLARE_HOOK(int,sb4s_register_doc,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_register_doc2,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_get_doc,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_set_docattr,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_delete_doc,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_delete_oid,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_search_doc,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_search2_doc,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_dispatch,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_status,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_last_docid,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_remote_morphological_analyze_doc,(int sockfd))

/* add khyang */
SB_DECLARE_HOOK(int,sb4s_status_str,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_set_log,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_help,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_rmas,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_indexwords,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_tokenizer,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_qpp,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_show_spool,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_get_cdm_size,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_get_abstract,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_get_field,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_undel_doc,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_config,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_get_wordid,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_get_new_wordid,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_get_docid,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_index_list,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_word_list,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_del_system_doc,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_systemdoc_count,(int sockfd))	
SB_DECLARE_HOOK(int,sb4s_did_req_comment,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_oid_req_comment,(int sockfd))
SB_DECLARE_HOOK(int,sb4s_get_oid_field,(int sockfd))

int TCPSendData(int sockfd, void *data, int len, int server_side);
int TCPRecvData(int sockfd, void *data, int *len, int server_side);

int TCPSend(int sockfd, header_t *header, void *data, int server_side);
int TCPRecv(int sockfd, header_t *header, void *data, int server_side);

int send_nak_str(int sockfd, char *error_string);

/* ******************************************* */
/*                 ERROR CODE                  */
/* ******************************************* */
/*   SYSTEM   */
#define SB4_ERS_JOB_NONE    (100)
#define SB4_ERS_FILE_OPEN   (101)
#define SB4_ERS_FILE_READ   (102)
#define SB4_ERS_FILE_WRITE  (103)
#define SB4_ERS_MEM_LACK    (104)
#define SB4_ERS_OBJ_NONE    (105)
#define SB4_ERS_SROP_NONE   (106)
#define SB4_ERS_OBJ_LACK    (107)
#define SB4_ERS_BUF_TOO_SMALL (108)
#define SB4_ERS_ORDER_FAULT (109)

/*   USSB4_ER   */
#define SB4_ERU_UN_NONE     (200)
#define SB4_ERU_UN_FAULT    (201)
#define SB4_ERU_UN_EXIST    (206)
#define SB4_ERU_UN_NO_EXIST (207)
#define SB4_ERU_UPW_NONE    (202)
#define SB4_ERU_UPW_FAULT   (203)
#define SB4_ERU_UID_NONE    (204)
#define SB4_ERU_UID_FAULT   (205)
#define SB4_ERU_DSC_FAULT   (208)
#define SB4_ERU_DELETED     (209)
#define SB4_ERU_ALIVE       (210)
#define SB4_ERU_USC_NONE    (211)
#define SB4_ERU_USC_FAULT   (212)
#define SB4_ERU_CSC_NONE    (213)
#define SB4_ERU_CSC_FAULT   (214)
#define SB4_ERU_DSC_NONE    (215)
#define SB4_ERU_VBM			(216)
#define SB4_ERU_DI			(217)

/*   CATEGORY   */
#define SB4_ERC_CID_NONE    (300)
#define SB4_ERC_CID_FAULT   (301)
#define SB4_ERC_CN_NONE     (302)
#define SB4_ERC_CN_FAULT    (303)
#define SB4_ERC_CN_EXIST    (306)
#define SB4_ERC_CN_NO_EXIST (304)
#define SB4_ERC_CPID_FAULT  (305)
#define SB4_ERC_CHILD_FULL  (307)
#define SB4_ERC_CSC_NONE    (308)
#define SB4_ERC_DENY_DEL    (309)
#define SB4_ERC_DELETED     (310)
#define SB4_ERC_ALIVE       (311)
#define SB4_ERC_CSC_FAULT   (312)
#define SB4_ERC_CPN_NONE    (313)
#define SB4_ERC_CPN_FAULT   (314)
#define SB4_ERC_CPID_NONE   (316)
#define SB4_ERC_DENY_RES    (317)

/*   DOCUMENT   */
#define SB4_ERD_INVALID		(499)

#define SB4_ERD_DID_NONE    (400)
#define SB4_ERD_DID_FAULT   (401)
#define SB4_ERD_OID_NONE    (402)
#define SB4_ERD_OID_FAULT   (403)
#define SB4_ERD_DIT_LEN     (404)
#define SB4_ERD_CT_NONE     (405)
#define SB4_ERD_CT_FAULT    (406)
#define SB4_ERD_OID_EXIST   (407)
#define SB4_ERD_DN_NONE     (408)
#define SB4_ERD_DTP_FAULT   (409)
#define SB4_ERD_TOO_BIG     (410)
#define SB4_ERD_DELETED     (411)
#define SB4_ERD_DENY_DEL    (412)
#define SB4_ERD_ALIVE       (413)
#define SB4_ERD_DSC_NONE    (414)
#define SB4_ERD_DN_FAULT    (415)
#define SB4_ERD_CT_FULL     (416)
#define SB4_ERD_TT_FAULT    (417)
#define SB4_ERD_DTR_FAULT   (418)
#define SB4_ERD_DTC_FAULT   (419)
#define SB4_ERD_AT_FAULT    (420)
#define SB4_ERD_KW_FAULT    (421)
#define SB4_ERD_FD_FAULT    (422)
#define SB4_ERD_DIT_FAULT	(423)

/*   WORD   */
#define SB4_ERW_NO_INDEX    (500)

/*  FUNTION  */
#define SB4_ERF_QU_NONE     (600)
#define SB4_ERF_CT_NONE     (601)
#define SB4_ERF_CT_FAULT    (602)
#define SB4_ERF_CATLST_FAULT (603)
#define SB4_ERF_WORD_NONE   (604)
#define SB4_ERF_DOC_NONE    (605)
#define SB4_ERF_PAGE_FAULT  (606)
#define SB4_ERF_SEND_WORD   (607)
#define SB4_ERF_BUF_TOO_SMALL   (608)
#define SB4_ERF_OID_TOO_LONG    (609)
#define SB4_ERF_OID_NONE    (610)
#define SB4_ERF_ARG_FAULT   (613)
#define SB4_ERF_DENY_LINKCC (614)
#define SB4_ERF_DENY_MOVECC (615)
#define SB4_ERF_DENY_UNLINKCC (616)
#define SB4_ERF_DENY_LINKDD (617)
#define SB4_ERF_DENY_UNLINKDD (618)
#define SB4_ERF_DENY_LINKDC (619)
#define SB4_ERF_DENY_UNLINKDC (620)
#define SB4_ERF_TOO_MANY_WORDS (621)

/*   TCP/IP   */
#define SB4_ERT_SOCKET      (700)
#define SB4_ERT_BIND        (701)
#define SB4_ERT_LISTEN      (702)
#define SB4_ERT_ACCEPT      (703)
#define SB4_ERT_CONNECT     (704)
#define SB4_ERT_SEND_DATA   (706)
#define SB4_ERT_RECV_DATA   (707)
#define SB4_ERT_SEND_FILE   (709)
#define SB4_ERT_RECV_FILE   (708)
#define SB4_ERT_SEND_FILEOFF    (710)
#define SB4_ERT_SEND_OPCD   (711)
#define SB4_ERT_RECV_OPCD   (712)
#define SB4_ERT_SEND_ACK    (716)
#define SB4_ERT_RECV_ACK    (717)

#endif
