/* $Id$ */
#include "softbot.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/docattr.h"
#include "mod_api/cdm.h"
#include "mod_api/xmlparser.h"

#include "mod_docattr_lgcaltex.h"
#include "mod_qp/mod_qp.h"
#include "mod_docattr/mod_docattr.h"
#include "mod_vrm/vrm.h"

#include <stdio.h>
#include <sys/types.h> /* waitpid(2) of ac_remake_main() */
#include <sys/wait.h>  /* waitpid(2) of ac_remake_main() */

#define MAX_ENUM_NUM		1024
#define MAX_ENUM_LEN		SHORT_STRING_SIZE

static char *constants[MAX_ENUM_NUM] = { NULL };
static long long constants_value[MAX_ENUM_NUM];

static long long return_constants_value(char *value, int valuelen);
static int get_item(char *src, char **info, char delimiter);

static char fieldRootName[MAX_FIELD_NAME_LEN] = "Document"; //XXX HACK!

/******************************************************************************
 * ACL(Access Control List) Things
 *
 */

static int get_policy_item(char *src, int type, lgcaltex_vrm_t* v, char* seps);

enum {
    TYPE_STRUCTURE,
    TYPE_DUTY,
    TYPE_PERSON,
    TYPE_TFT
};

#define	AC_PATH "dat/cdm/lgcaltex_ac"
#define MAX_ACFIELD_LEN 4
static char ac_field_name[MAX_ACFIELD_LEN][16] =
{
    "Structure",
    "Duty",
    "Person",
    "TFT"
};

static vrm_t* ac_list;

static int ac_write(int docid, lgcaltex_vrm_t* v) {
    return sb_run_vrm_add(ac_list, docid, v->policy, v->count*sizeof(lgcaltex_policy_t)); 
}

static int ac_read(int docid, lgcaltex_vrm_t* v) {
	void* data=0x00;
    int size;
	
    if(sb_run_vrm_read(ac_list, docid, &data, &size) != SUCCESS) {
		return FAIL;
	}
    
    SB_DEBUG_ASSERT(size % sizeof(lgcaltex_policy_t) == 0);

	v->policy = (lgcaltex_policy_t*)data;
    v->count  = size / sizeof(lgcaltex_policy_t);

	return SUCCESS;
}

/* vrm open */
static int init(void) {
	sb_assert(sizeof(lgcaltex_attr_t) == 64);

	if(sb_run_vrm_open(AC_PATH, &ac_list) != SUCCESS)
		return FAIL;
    else
	    return SUCCESS;
}

static int ac_close(void) {
    if(sb_run_vrm_close(ac_list) != SUCCESS)
		return FAIL;
    else
	    return SUCCESS;
}

static char* _trim(char *str, int *len)
{
    char *tmp, *start;

    if (*len == 0) return str;

    start = str;
    for (tmp=str; tmp<str+*len; tmp++) {
        if (*tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r') {
            start = tmp;
            break;
        }
    }
    if (tmp == str + *len) {
        *len = 0;
        return str;
    }
    for (tmp=str+*len-1; tmp>=start; tmp--) {
        if (*tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r') {
            break;
        }
    }
    *len = tmp - start + 1;
    return start;
}

/* vrm add */
static int put_doc_ac(void* did_db, char *oid, uint32_t *registeredDocId, VariableBuffer *pCannedDoc)
{
    parser_t *p;
    field_t *f;
    docattr_t docattr;
    char path[STRING_SIZE];
    int i, ret, iSize=0, iResult=0;
    lgcaltex_vrm_t lgcaltex_vrm;
    lgcaltex_policy_t lgcaltex_policy[MAX_POLICY];

	/* aCannedDoc is a "static" variable. no need to care memory leaking.
	 * --aragorn, 2004/01/27 */
    static char aCannedDoc[DOCUMENT_SIZE];
#if 0
    static char *aCannedDoc = NULL;

    if (aCannedDoc == NULL) {
        aCannedDoc = (char *)sb_malloc(DOCUMENT_SIZE);
        if (aCannedDoc == NULL) {
            crit("out of memory: %s", strerror(errno));
			return FAIL;
        }
    	info("malloc-ed can is %p, length is %d", aCannedDoc, DOCUMENT_SIZE);
    }
#endif

    lgcaltex_vrm.count = 0;
    lgcaltex_vrm.policy = lgcaltex_policy;

    // get canned doc from variable buffer
    iSize = sb_run_buffer_getsize(pCannedDoc);
    if (iSize < 0) {
        error("error in buffer_getsize()");
		return FAIL;
    }

    if (iSize >= DOCUMENT_SIZE) {
        warn("size of document[%u] exceed maximum limit; fail to register doc",
                *registeredDocId);
		return FAIL;
    }

    iResult = sb_run_buffer_get(pCannedDoc, 0, iSize, aCannedDoc);
    if (iResult < 0) {
        error("error in buffer_get()");
		return FAIL;
    }
    aCannedDoc[iSize] = '\0';

    p = sb_run_xmlparser_parselen("CP949", aCannedDoc, iSize);
    if (p == NULL) {
        error("2. cannot parse document");
		return FAIL;
    }

    DOCATTR_SET_ZERO(&docattr);
    for (i=0; i<MAX_ACFIELD_LEN && ac_field_name[i]; i++) {
        char *val, value[STRING_SIZE];
        int len;

        strcpy(path, "/");
        strcat(path, fieldRootName);
        strcat(path, "/");
        strcat(path, ac_field_name[i]);

        f = sb_run_xmlparser_retrieve_field(p, path);
        if (f == NULL) {
            warn("cannot get field[/%s/%s] of ducument[%s] (path:%s)", 
                    fieldRootName, 
                    ac_field_name[i], oid, path);
            continue;
        }

        len = f->size;
        val = _trim(f->value, &len);
        len = (len > STRING_SIZE-1) ? STRING_SIZE-1 : len;
        strncpy(value, val, len);
        value[len] = '\0';

        if (f->size == 0) {
            continue;
        }
       
        debug("f->value : [%s], docattrField[%s]", value, ac_field_name[i]);
        get_policy_item(value, i, &lgcaltex_vrm, ":"); 
    }
    sb_run_xmlparser_free_parser(p);

    for(i=0; i < lgcaltex_vrm.count; i++) {
        switch(lgcaltex_vrm.policy[i].type) {
        case TYPE_STRUCTURE:
            debug("[%d]structure:[%s]", i, lgcaltex_vrm.policy[i].policy.structure);
            break;
        case TYPE_DUTY:
            debug("[%d]duty:[%d]", i, lgcaltex_vrm.policy[i].policy.duty);
            break;
        case TYPE_PERSON:
            debug("[%d]person:[%s]", i, lgcaltex_vrm.policy[i].policy.person);
            break;
        case TYPE_TFT:
            debug("[%d]tft:[%s]", i, lgcaltex_vrm.policy[i].policy.tft);
            break;
        default:
            warn("unknown type [%d]", lgcaltex_vrm.policy[i].type);
            break;
        }
    }
	ret =  ac_write(*registeredDocId, &lgcaltex_vrm);

	return ret;
}

#if 0
static void print_docattr(lgcaltex_attr_t* v, int docid) {
    if(v == NULL) return;

    debug("docid[%d] : StrYN[%d], DutyYN[%d], PerYN[%d], TFTYN[%d]", docid, v->StrYN, v->DutyYN, v->PerYN, v->TFTYN);
}
#endif

//vrm read
#define AC_NO  (0)
#define AC_YES (1)
static int compare_function_ac(void *dest, void *cond, uint32_t docid) {
	int i;
	lgcaltex_vrm_t lgcaltex_vrm;
	static char ac_string[2][7] = {"AC_NO", "AC_YES"};

	lgcaltex_attr_t *docattr = (lgcaltex_attr_t*)dest;
	lgcaltex_cond_t *doccond = (lgcaltex_cond_t*)cond;

	if (doccond->delete_check == 0 && docattr->is_deleted) return AC_NO;

	if (doccond->Date1_check == 1) {
		if (doccond->Date1_start > docattr->Date1 ||
				doccond->Date1_finish < docattr->Date1)
			return AC_NO;
	}

	if (doccond->Date2_check == 1) {
		if (doccond->Date2_start > docattr->Date2 ||
				doccond->Date2_finish < docattr->Date2)
			return AC_NO;
	}

	if (doccond->SystemName_check > 0 && doccond->SystemName[0] != 255) {
		for (i=0; i<doccond->SystemName_check; i++) {
			if (doccond->SystemName[i] == docattr->SystemName) break;
		}
		if (i == doccond->SystemName_check) return AC_NO;
	}
	
	if (doccond->Part_check > 0 && doccond->Part[0] != 255) {
		for (i=0; i<doccond->Part_check; i++) {
			if (doccond->Part[i] == docattr->Part) break;
		}
		if (i == doccond->Part_check) return AC_NO;
	}

	if (doccond->AppFlag_check == 1 && doccond->AppFlag != docattr->AppFlag) {
		debug("doccond->AppFlag[%d] != docattr->AppFlag[%d], return AC_NO",
				doccond->AppFlag, docattr->AppFlag);
		return AC_NO;
	}
	
	if (doccond->FileExt_check == 1 && doccond->FileExt != docattr->FileExt) {
		return AC_NO;
	}
	
	if ( doccond->Super_User == 255 ) {
		debug("user is a Super_User, return AC_YES");
		return AC_YES;
	}
		
	/*보안 알고리즘 추가 */
	if (docattr->SC == 1)
	{
        int k,j, accessable=0;

		if (docattr->TFTYN == 0 && docattr->PerYN == 0 && docattr->DutyYN == 0 && docattr->StrYN == 0) {
            warn("docid[%d] sc field on, but all access control fields off, return AC_YES", docid);
            return AC_YES;
        }

        if(ac_read(docid, &lgcaltex_vrm) != SUCCESS) {
		warn("cannot read ac_read(%d, &lgcaltex_vrm), return AC_NO: %s", docid, strerror(errno));
    		return AC_NO;
    	}

    	for(i=0; i < lgcaltex_vrm.count; i++) {
    	    switch(lgcaltex_vrm.policy[i].type) {
    	    case TYPE_STRUCTURE:
    	    	debug("[%d]structure:[%s]", i, lgcaltex_vrm.policy[i].policy.structure);
    	    	break;
    	    case TYPE_DUTY:
    	    	debug("[%d]duty:[%d]", i, lgcaltex_vrm.policy[i].policy.duty);
    	    	break;
    	    case TYPE_PERSON:
    	    	debug("[%d]person:[%s]", i, lgcaltex_vrm.policy[i].policy.person);
    	    	break;
    	    case TYPE_TFT:
    	    	debug("[%d]tft:[%s]", i, lgcaltex_vrm.policy[i].policy.tft);
    	    	break;
            default:
                warn("unknown type [%d]", lgcaltex_vrm.policy[i].type);
                break;
    		}
    	}

		/* TFT 보안 : 사용자 TFT중 하나라도 문서 TFT에 존재시 검색대상이 됨. 없으면 다음 조건 비교. */
		if (docattr->TFTYN == 1 && doccond->TFT_cnt > 0) {
			
			for(k=0; k<doccond->TFT_cnt; k++)
			{
                for(j=0; j < lgcaltex_vrm.count; j++) {
                    if(lgcaltex_vrm.policy[j].type == TYPE_TFT) {
                        if(strncmp(doccond->TFT[k], lgcaltex_vrm.policy[j].policy.tft, TFT_CODE_LEN) == 0) {
							debug("user's tft[%s] is matched in doc's tft[%s], return AC_YES",
                                  doccond->TFT[k], lgcaltex_vrm.policy[j].policy.tft);
                            return AC_YES;
						}
                    }
                }
			}
		}

		/* 사번 보안 : 사용자 사번이 문서 사번에 존재시 검색대상이 됨. 없으면 다음 조건 비교. */
		if (docattr->PerYN == 1 && doccond->Person != NULL) {
            for(j=0; j < lgcaltex_vrm.count; j++) {
                if(lgcaltex_vrm.policy[j].type == TYPE_PERSON) {
                    if(strncmp(doccond->Person, lgcaltex_vrm.policy[j].policy.person, PERSON_CODE_LEN) == 0) {
						debug("users's person[%s] is matched in doc's person[%s], return AC_YES",
							  doccond->Person, lgcaltex_vrm.policy[j].policy.person);
                        return AC_YES;
					}
                }
            }
		}

		/* 직급보안 - 자기보다 작으면 안 보여줌 */
		if (docattr->DutyYN == 1) {
			if ( doccond->Duty_cnt == 0)
				return AC_NO;
		    
            for(j=0; j < lgcaltex_vrm.count; j++) {
                if(lgcaltex_vrm.policy[j].type == TYPE_DUTY) {
                    if(doccond->Duty > lgcaltex_vrm.policy[j].policy.duty) {
                        return AC_NO;
					}
                }
            }
			debug("duty[%d] is matched. checking structure...", lgcaltex_vrm.policy[j].policy.duty);
            accessable=1;
		}

		/* 조직 보안 - 10자리 조직 enum값으로 비교 */
		if (docattr->StrYN == 1) {
            long long q_structure=0, d_structure=0;
			
			if ( doccond->Structure_cnt == 0 ) {
				debug("no structure info in doccond, return AC_NO");
				return AC_NO;
			}
		    
            for(k=0; k<doccond->Structure_cnt; k++) {
                q_structure = return_constants_value(doccond->Structure[k], strlen(doccond->Structure[k]));		

                for(j=0; j < lgcaltex_vrm.count; j++) {
                    if(lgcaltex_vrm.policy[j].type == TYPE_STRUCTURE) {
                        d_structure = return_constants_value(lgcaltex_vrm.policy[j].policy.structure, 
										                     strlen(lgcaltex_vrm.policy[j].policy.structure));

                        while((d_structure & 0xff) == 0) {
                            d_structure = d_structure >> 8;
                            q_structure = q_structure >> 8;

                            if(d_structure == 0) break;
                        } 
    
                        if(d_structure == q_structure) {
						    debug("d_structure[%lld] == q_structure[%lld]", d_structure, q_structure);
						    debug("user's structure[%s] is matched in doc's structure[%s], return AC_YES",
									doccond->Structure[k], lgcaltex_vrm.policy[j].policy.structure);
                            return AC_YES; 
						}
                    }
                }
            }

		} else { 
			debug("no structure policy, return %s", ac_string[accessable]);
			return accessable; //직급보안은 통과하고 조직보안이 존재하지 않을때.
		}

        debug("structure policy is not matched, return %s", ac_string[0]);
    	return AC_NO;
    } 

	debug("no security policy, return AC_YES");
    return AC_YES;
}

/*****************************************************************************/

static int compare_function_for_qsort(const void *dest, const void *sour, void *userdata)
{
	int i, diff;
	docattr_sort_t *sh;
	lgcaltex_attr_t attr1, attr2;
	int hit1=0, hit2=0;

	if (sb_run_docattr_get(((doc_hit_t *)dest)->id, &attr1) < 0) {
		error("cannot get docattr element");
		return 0;
	}
	if (sb_run_docattr_get(((doc_hit_t *)sour)->id, &attr2) < 0) {
		error("cannot get docattr element");
		return 0;
	}
	

	sh = (docattr_sort_t *)userdata;
	for (i=0; 
		 i<MAX_SORTING_CRITERION && sh->keys[i].key[0]!='\0'; 
		 i++) {

		switch (sh->keys[i].key[0]) {
			case '0': // HIT
				hit1 = ((doc_hit_t *)dest)->hitratio;
				hit2 = ((doc_hit_t *)sour)->hitratio;
				
				if ((diff = hit1- hit2) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '1': // Title
				if ((diff = hangul_strncmp(attr1.Title, attr2.Title, 16)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '2': // Author
				if ((diff = hangul_strncmp(attr1.Author, attr2.Author, 16)) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
			case '3': // Date1
				if ((diff = attr1.Date1 - attr2.Date1) == 0) {
					break;
				}
				else {
					diff = diff > 0 ? 1 : -1;
					return diff * sh->keys[i].order;
				}
				break;
		}
	}

	return 0;
}

/*
 * NOTICE:
 * 	if success, return SUCCESS(1)
 */
static int mask_function(void *dest, void *mask) {
	lgcaltex_attr_t *docattr = (lgcaltex_attr_t *)dest;
	lgcaltex_mask_t *docmask = (lgcaltex_mask_t *)mask;
	

	if (docmask->delete_mark)
		docattr->is_deleted = 1;

	if (docmask->undelete_mark)
		docattr->is_deleted = 0;

	if (docmask->set_SystemName)
		docattr->SystemName = docmask->SystemName;

	if (docmask->set_Part)
		docattr->Part = docmask->Part;

	if (docmask->set_AppFlag)
		docattr->AppFlag = docmask->AppFlag;

	if (docmask->set_FileExt)
		docattr->FileExt = docmask->FileExt;

	if (docmask->set_SC)
		docattr->SC = docmask->SC;
	
	if (docmask->set_StrYN)
		docattr->StrYN = docmask->StrYN;
	
	if (docmask->set_DutyYN)
		docattr->DutyYN = docmask->DutyYN;
	
	if (docmask->set_PerYN)
		docattr->PerYN = docmask->PerYN;
	
	if (docmask->set_TFTYN)
		docattr->TFTYN = docmask->TFTYN;
		
	if (docmask->set_Date1)
		docattr->Date1 = docmask->Date1;
	
	if (docmask->set_Date2)
		docattr->Date2 = docmask->Date2;
		
	if (docmask->set_Title)
		memcpy(docattr->Title, docmask->Title, 16);
		
	if (docmask->set_Author)
		memcpy(docattr->Author, docmask->Author, 16);
		
	return 1;
}

/* if fail, return -1 : 등록시 doc에 대한 docattr 설정 */
static int set_docattr_function(void *dest, char *key, char *value)
{
	lgcaltex_attr_t *docattr = (lgcaltex_attr_t *)dest;
	
    debug("key[%s] : value[%s]", key, value);

	if (strcasecmp(key, "Delete") == 0) {
		docattr->is_deleted = 1;
	}
	else if (strcasecmp(key, "SystemName") == 0) {
		docattr->SystemName = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "Part") == 0) {
		docattr->Part = 
			(uint8_t)return_constants_value(value, strlen(value));
	}
	else if (strcasecmp(key, "AppFlag") == 0) {
		docattr->AppFlag = (uint8_t)atoi(value);
	}
	else if (strcasecmp(key, "FileName") == 0) {
		char *str;
		str = strrchr(value,'.');
		if ( str == NULL ) str = value;

		docattr->FileExt = 
			(uint8_t)return_constants_value(str, strlen(str));
	}
	else if (strcasecmp(key, "Duty") == 0) {
		if ( strlen(value) == 0 )
			docattr->DutyYN = 0;
		else
			docattr->DutyYN = 1;
			
	}
	else if (strcasecmp(key, "SC") == 0) {
		docattr->SC = (uint8_t)atoi(value);
	}
	else if (strcasecmp(key, "Structure") == 0) {
		if ( strlen(value) == 0 )
			docattr->StrYN = 0;
		else
			docattr->StrYN = 1;
	}
	else if (strcasecmp(key, "Person") == 0) {
		if ( strlen(value) == 0 )
			docattr->PerYN = 0;
		else
			docattr->PerYN = 1;
	}
	else if (strcasecmp(key, "TFT") == 0) {
		if ( strlen(value) == 0 )
			docattr->TFTYN = 0;
		else
			docattr->TFTYN = 1;
	}
	else if (strcasecmp(key, "Date1") == 0) {
		docattr->Date1 = (uint32_t)atol(value);
	}
	else if (strcasecmp(key, "Date2") == 0) {
		docattr->Date2 = (uint32_t)atol(value);
	}
	else if (strcasecmp(key, "Title") == 0) {
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];

		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
		sb_run_index_word_extractor_set_text(ex, _value);

		sb_run_get_index_words(ex, &word, 16);
		strncpy(docattr->Title, word.word, 16);
	}
	else if (strcmp(key, "Author") == 0) {  
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];

		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
		sb_run_index_word_extractor_set_text(ex, _value);

		sb_run_get_index_words(ex, &word, 16);
		strncpy(docattr->Author, word.word, 16);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int get_docattr_function(void *dest, char *key, char *buf, int buflen)
{
	lgcaltex_attr_t *docattr = (lgcaltex_attr_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		snprintf(buf, buflen, "%u",docattr->is_deleted);
	}
	else if (strcasecmp(key, "SystemName") == 0) {
		snprintf(buf, buflen, "%u",docattr->SystemName);
	}
	else if (strcasecmp(key, "Part") == 0) {
		snprintf(buf, buflen, "%u",docattr->Part);
	}
	else if (strcasecmp(key, "AppFlag") == 0) {
		snprintf(buf, buflen, "%u",docattr->AppFlag);
	}
	else if (strcasecmp(key, "FileExt") == 0) {
		snprintf(buf, buflen, "%u",docattr->FileExt);
	}
	else if (strcasecmp(key, "SC") == 0) {
		snprintf(buf, buflen, "%u",docattr->SC);
	}
	else if (strcasecmp(key, "StrYN") == 0) {
		snprintf(buf, buflen, "%u",docattr->StrYN);
	}
	else if (strcasecmp(key, "DutyYN") == 0) {
		snprintf(buf, buflen, "%u",docattr->DutyYN);
	}
	else if (strcasecmp(key, "PerYN") == 0) {
		snprintf(buf, buflen, "%u",docattr->PerYN);
	}
	else if (strcasecmp(key, "TFTYN") == 0) {
		snprintf(buf, buflen, "%u",docattr->TFTYN);
	}
	else if (strcasecmp(key, "Date1") == 0) {
		snprintf(buf, buflen, "%u",docattr->Date1);
	}
	else if (strcasecmp(key, "Date2") == 0) {
		snprintf(buf, buflen, "%u",docattr->Date2);
	}
	else if (strcasecmp(key, "Title") == 0) {
		snprintf(buf, buflen, "%s",docattr->Title);
	}
	else if (strcasecmp(key, "Author") == 0) {
		snprintf(buf, buflen, "%s",docattr->Author);
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}
	return 1;
}

/* if fail, return -1 : 검색 조건 set */
static int set_doccond_function(void *dest, char *key, char *value)
{
	lgcaltex_cond_t *doccond = (lgcaltex_cond_t *)dest;
	/* length(value) should be shorter than STRING_SIZE(=256) */
	char my_value[STRING_SIZE];

	INFO("key:%s, value:%s", key, value);
	strncpy(my_value, value, STRING_SIZE); my_value[STRING_SIZE-1] = '\0';

	if (strcasecmp(key, "Delete") == 0) {
		doccond->delete_check = atoi(value);
		return 1;
	}

	if (strcasecmp(key, "SystemName") == 0) {
		int  count;	
		char *token;

		token = strtok(my_value, ",");
		for ( count = 0; count < MAX_SYSTEM_NAME_COND; count++ )
		{
			if (token == NULL) break;
			doccond->SystemName[count] = 
				return_constants_value(token, strlen(token));
			token = strtok(NULL, ",");
		}
		doccond->SystemName_check = count;
	}
	else if (strcasecmp(key, "Part") == 0) {
		int  count;	
		char *token;

		token = strtok(my_value, ",");
		for ( count = 0; count < MAX_PART_COND; count++ )
		{
			if (token == NULL) break;

			doccond->Part[count] = 
				return_constants_value(token, strlen(token));
			token = strtok(NULL, ",");
		}
		doccond->Part_check = count;
	}
	else if (strcasecmp(key, "AppFlag") == 0) {
		doccond->AppFlag = (uint8_t)atoi(value);
		doccond->AppFlag_check = 1;
	}
	else if (strcasecmp(key, "FileName") == 0) {
		char *str;
		str = strrchr(value,'.');
		if ( str == NULL ) str = value;

		doccond->FileExt = 
			(uint8_t)return_constants_value(str, strlen(str));
		doccond->FileExt_check = 1;
	}
	else if (strcasecmp(key, "Duty") == 0) {
		int  count;
		char *token;
		int  duty, min_duty = 255;

		token = strtok(my_value, ",");
		for ( count = 0; ; count++ )
		{
			if (token == NULL) break;
			duty = return_constants_value(token, strlen(token));
			if (duty < min_duty) min_duty = duty;

			token = strtok(NULL, ",");
		}

		doccond->Duty = min_duty;
		doccond->Duty_check = 1;
		doccond->Duty_cnt=1;
	}
	else if (strcmp(key, "Structure") == 0) {
		int  count, len;	
		char *token;

		token = strtok(my_value, ",");
		for ( count = 0; count < MAX_STRUCTURE_COND; count++ )
		{
			if (token == NULL) break;
			len = strlen(token);

			doccond->Structure[count] = (char*)sb_calloc(len+1, sizeof(char));
			strncpy(doccond->Structure[count], token, len);
			doccond->Structure[count][len] = '\0';

			token = strtok(NULL, ",");
		}
		doccond->Structure_check = 1;
		doccond->Structure_cnt = count;
	}
	else if (strcmp(key, "TFT") == 0) {
		int  count, len;	
		char *token;

		token = strtok(my_value, ",");
		for ( count = 0; count < MAX_TFT_COND; count++ )
		{
			if (token == NULL) break;
			len = strlen(token);

			doccond->TFT[count] = (char*)sb_calloc(len+1, sizeof(char));
			strncpy(doccond->TFT[count], token, len);
			doccond->TFT[count][len] = '\0';

			token = strtok(NULL, ",");
		}
		doccond->TFT_check = 1;
		doccond->TFT_cnt = count;
	}
	else if (strcmp(key, "SUPER") == 0) {
		doccond->Super_User = (uint8_t)return_constants_value(value, strlen(value));;
	}
	else if (strcmp(key, "Person") == 0) {
		strncpy(doccond->Person, value, SHORT_STRING_SIZE);
		doccond->Person[SHORT_STRING_SIZE-1] = '\0';
		doccond->Person_check = 1;
	}
	else if (strcmp(key, "Date1") == 0) {
		int n = sscanf(value, "%u-%u", &(doccond->Date1_start), &(doccond->Date1_finish));
		if (n != 2) {
			warn("wrong docattr query: Date1");
			return -1;
		}
	    doccond->Date1_check = 1;
	}
	else if (strcmp(key, "Date2") == 0) {
		int n = sscanf(value, "%u-%u", &(doccond->Date2_start), &(doccond->Date2_finish));
		if (n != 2) {
			warn("wrong docattr query: Date2");
			return -1;
		}
	    doccond->Date2_check = 1;
	}
	else {
		warn("no such a field in docattr db: %s", key);
		return -1;
	}

	return 1;
}

static int set_docmask_function(void *dest, char *key, char *value)
{
	lgcaltex_mask_t *docmask = (lgcaltex_mask_t *)dest;

	if (strcasecmp(key, "Delete") == 0) {
		docmask->delete_mark = 1;
	}
    else if (strcasecmp(key, "Undelete") == 0) {
       	docmask->undelete_mark = 1;
    }
    else if (strcasecmp(key, "SystemName") == 0) {
		docmask->SystemName = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_SystemName = 1;
	}
	else if (strcasecmp(key, "Part") == 0) {
		docmask->Part = 
			(uint8_t)return_constants_value(value, strlen(value));
		docmask->set_Part = 1;
	}
	else if (strcasecmp(key, "AppFlag") == 0) {
		docmask->AppFlag = (uint8_t)atoi(value);
		docmask->set_AppFlag = 1;
	}
	else if (strcasecmp(key, "FileName") == 0) {
		char *str;
		str = strrchr(value,'.');
		if ( str == NULL ) str = value;

		docmask->FileExt = 
			(uint8_t)return_constants_value(str, strlen(str));
		docmask->set_FileExt = 1;
	}
	else if (strcasecmp(key, "SC") == 0) {
		docmask->SC = (uint8_t)atoi(value);
		docmask->set_SC = 1;
	}
	else if (strcasecmp(key, "StrYN") == 0) {
		docmask->StrYN = (uint8_t)atoi(value);
		docmask->set_StrYN = 1;
	}
	else if (strcasecmp(key, "DutyYN") == 0) {
		docmask->DutyYN = (uint8_t)atoi(value);
		docmask->set_DutyYN = 1;
	}
	else if (strcasecmp(key, "PerYN") == 0) {
		docmask->PerYN = (uint8_t)atoi(value);
		docmask->set_PerYN = 1;
	}
	else if (strcasecmp(key, "TFTYN") == 0) {
		docmask->TFTYN = (uint8_t)atoi(value);
		docmask->set_TFTYN = 1;
	}
	else if (strcasecmp(key, "Date1") == 0) {
		docmask->Date1 = (uint32_t)atol(value);
		docmask->set_Date1 = 1;
	}
	else if (strcasecmp(key, "Date2") == 0) {
		docmask->Date2 = (uint32_t)atol(value);
		docmask->set_Date2 = 1;
	}
	else if (strcasecmp(key, "Title") == 0) {
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];

		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
		sb_run_index_word_extractor_set_text(ex, _value);

		sb_run_get_index_words(ex, &word, 16);
		strncpy(docmask->Title, word.word, 16);
		docmask->set_Title = 1;
	}
	else if (strcasecmp(key, "Author") == 0) {
		/* chinese -> hangul */
		char _value[SHORT_STRING_SIZE];

		index_word_extractor_t *ex = NULL;
		index_word_t word;

		strncpy(_value, value, SHORT_STRING_SIZE-1);
		_value[SHORT_STRING_SIZE-1] = '\0';

		ex = sb_run_new_index_word_extractor(2);
		if (ex == NULL || ex == (index_word_extractor_t*)MINUS_DECLINE) return FAIL;
		sb_run_index_word_extractor_set_text(ex, _value);

		sb_run_get_index_words(ex, &word, 16);
		strncpy(docmask->Author, word.word, 16);
		docmask->set_Author = 1;
	}
	else {
		warn("there is no such field[%s]", key);
	}
	return 1;
}
 
/* src : 123,23,45 
    int return : count
    info : 123 
    delimiter: split(,) */
/**
 * get_list
 * @param src delimiter로 entry가 구분된 source string. 함수를 호출하면 NULL문자가 삽입되어
 *            문자열이 변경됨.
 * @param list source string의 각  입력되어 구분됨.
 * @
 */
static int get_item(char *src, char **info, char delimiter)
{
	char *start, *end,  *c;
	int cnt=0, i=0;

	while ((c = strchr(src, delimiter)) != NULL) {
				*c = ':';
				cnt++;
	}
	cnt++;
	
	end = src;
	
	for (i=0; i<cnt; i++)
	{
		info[i] = (char*)sb_calloc(1, STRING_SIZE);
	
		start = strchr(end, ':');
		if ( start == NULL ) 
		{
			strcpy(info[i], end);
			return cnt;
		}
	
		
		*start = '\0';
	
		strcpy(info[i], end);
		end = start+1;
	}

	return cnt;
}

/* if fail, return 0 */
static long long return_constants_value(char *value, int valuelen)
{
	int i;
	for (i=0; i<MAX_ENUM_NUM && constants[i]; i++) {
		if (strncmp(value, constants[i], MAX_ENUM_LEN) == 0) {
			break;
		}
	}
	if (i == MAX_ENUM_NUM || constants[i] == NULL) {
		return 0;
	}
	return constants_value[i];
}

static void get_enum(configValue v)
{
	int i;
    char* p = 0x00;

	static char enums[MAX_ENUM_NUM][MAX_ENUM_LEN];
	for (i=0; i<MAX_ENUM_NUM && constants[i]; i++);
	if (i == MAX_ENUM_NUM) {
		error("too many constant is defined");
		return;
	}
	strncpy(enums[i], v.argument[0], MAX_ENUM_LEN);
	enums[i][MAX_ENUM_LEN-1] = '\0';
	constants[i] = enums[i];

    /* 세번째 인자 base(진법?) 가 0이면, 문자열의 처음 2글자에 따라 
     * base가 결정된다.
     * "0x1234..." -> 16진수
     * "01234..."  -> 8진수
     * "1234..."   -> 10진수
     */ 
	constants_value[i] = strtoll(v.argument[1], &p, 0);

    if(*p != 0x00) {
       warn("find invalid character[%s]", p);
    }

	INFO("Enum[%s]: %lld", constants[i], constants_value[i]);
}

static int get_policy_item(char *src, int type, lgcaltex_vrm_t* v, char* seps)
{
	char* token;
	int count=0;

	token = strtok(src, seps);

    while(token) {
    	count++;

    	if(v->count >= MAX_POLICY) {
    	    warn("full policy count [%d]", v->count);
    	    break;
    	}

		if(strlen(token) <= 0) {
			token = strtok(0x00,seps);
			continue;
		}
    	
		v->policy[v->count].type = type;
	    switch(type) {
	    case TYPE_STRUCTURE:
	    	strncpy(v->policy[v->count++].policy.structure, token, STRUCTURE_CODE_LEN);
	    	break;
	    case TYPE_DUTY:
	    	v->policy[v->count++].policy.duty = return_constants_value(token, strlen(token));
	    	break;
	    case TYPE_PERSON:
	    	strncpy(v->policy[v->count++].policy.person, token, PERSON_CODE_LEN);
	    	break;
	    case TYPE_TFT:
	    	strncpy(v->policy[v->count++].policy.tft, token, TFT_CODE_LEN);
	    	break;
        default:
            warn("unknown type[%d]", type);
            break;
		}
		
    	token = strtok(0x00,seps);
    }
	
	return count;
}

static config_t config[] = {
	CONFIG_GET("Enum", get_enum, 2, "constant"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_docattr_compare_function(compare_function_ac, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_mask_function(mask_function, NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_sort_function(compare_function_for_qsort, NULL, NULL, HOOK_MIDDLE);

	sb_hook_docattr_set_docattr_function(set_docattr_function,
			NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_get_docattr_function(get_docattr_function,
			NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_doccond_function(set_doccond_function,
			NULL, NULL, HOOK_MIDDLE);
	sb_hook_docattr_set_docmask_function(set_docmask_function,
			NULL, NULL, HOOK_MIDDLE);

	sb_hook_server_canneddoc_put_with_oid(put_doc_ac,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_close(ac_close,NULL,NULL,HOOK_MIDDLE);
}

module docattr_lgcaltex_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	init,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};

static int remake_ac(int start_docid, int count) {
	int i, k, ret;
	parser_t *p = NULL;

#define SMALL_DOCUMENT_SIZE (1024*10)
#ifdef TEST__AIX5
	char aCannedDoc[SMALL_DOCUMENT_SIZE];
#else
	static char *aCannedDoc = NULL;
#endif

	debug("before vrm_open(%s)", AC_PATH);
    if(sb_run_vrm_open(AC_PATH, &ac_list) != SUCCESS)
        return 1;
	debug("after vrm_open(%s)", AC_PATH);

#ifdef TEST__AIX5
	debug("this is TEST__AIX5");
#else
    if (aCannedDoc == NULL) {
        aCannedDoc = (char *)sb_malloc(DOCUMENT_SIZE);
        if (aCannedDoc == NULL) {
            crit("out of memory: %s", strerror(errno));
            return FAIL;
       }
    }
#endif

    for(i=start_docid; i <= (start_docid + count); i++) {
        lgcaltex_vrm_t lgcaltex_vrm;
        lgcaltex_policy_t lgcaltex_policy[MAX_POLICY];

		VariableBuffer var;
		int size, len, j;
		field_t *f;
        char *val, value[STRING_SIZE], path[STRING_SIZE];

        lgcaltex_vrm.count = 0;
        lgcaltex_vrm.policy = lgcaltex_policy;


		if ((i%100) == 0) info("rebuilding access control policy of docid[%d]", i);
		else debug("rebuilding access control policy of docid[%d]", i);

		sb_run_buffer_initbuf(&var);
		ret = sb_run_server_canneddoc_get(i,&var);
        //ret = sb_run_doc_get(i, &doc);
        if(ret < 0) {
            info("can't get canneddoc, docid[%d]", i);
			sb_run_buffer_freebuf(&var);
            continue;
        }
		debug("sb_run_doc_get[%d] return %d", i, ret);

        size = sb_run_buffer_getsize(&var);
        if (size >= DOCUMENT_SIZE) {
        //if (size > SMALL_DOCUMENT_SIZE) {
            sb_run_buffer_freebuf(&var);
            continue;
        }

        sb_run_buffer_get(&var, 0, size, aCannedDoc);
        sb_run_buffer_freebuf(&var);
        aCannedDoc[size] = '\0';

        p = sb_run_xmlparser_parselen("CP949", aCannedDoc, size);
        if (p == NULL) {
            error("cannot parse document[%d]", i);
            continue;
        }

        for(j=0; j<MAX_ACFIELD_LEN && ac_field_name[j]; j++) {
            strcpy(path, "/Document/");
            strcat(path, ac_field_name[j]);
			debug("ac_field_name[%d]:%s", j, ac_field_name[j]);

            f = sb_run_xmlparser_retrieve_field(p, path);
            if (f == NULL) {
                warn("cannot get field[/%s/%s] of ducument[%d] (path:%s)",
                        "Document",
                        ac_field_name[j], i, path);
                continue;
            }

            len = f->size;
            val = _trim(f->value, &len);
            len = (len>STRING_SIZE-1)?STRING_SIZE-1:len;
            strncpy(value, val, len);
            value[len] = '\0';

            if (len == 0) {
                continue;
            }

			debug("value:%s", value);
            get_policy_item(value, j, &lgcaltex_vrm, ":");            
        }

        for(k=0; k < lgcaltex_vrm.count; k++) {
            switch(lgcaltex_vrm.policy[k].type) {
            case TYPE_STRUCTURE:
                debug("[%d]structure:[%s]", k, lgcaltex_vrm.policy[k].policy.structure);
                break;
            case TYPE_DUTY:
                debug("[%d]duty:[%d]", k, lgcaltex_vrm.policy[k].policy.duty);
                break;
            case TYPE_PERSON:
                debug("[%d]person:[%s]", k, lgcaltex_vrm.policy[k].policy.person);
                break;
            case TYPE_TFT:
                debug("[%d]tft:[%s]", k, lgcaltex_vrm.policy[k].policy.tft);
                break;
            default:
                warn("unknown type [%d]", lgcaltex_vrm.policy[k].type);
                break;
            }
        }
		debug("before sb_run_vrm_add()");
        ret = sb_run_vrm_add(ac_list, i, lgcaltex_vrm.policy, lgcaltex_vrm.count*sizeof(lgcaltex_policy_t)); 
        if(ret != SUCCESS) {
            warn("fail ac_write, docid[%d]", i);
        }
	}

	sb_run_xmlparser_free_parser(p);

    if(sb_run_vrm_close(ac_list) != SUCCESS)
        return 1;

    return 0;
}

static int ac_remake_main(slot_t *slot) {
    int last_registered_docid;
    pid_t pid;
    int i, status, ret;

    info("start ac_remake........................");

	ret = sb_run_server_canneddoc_init();
	if ( ret != SUCCESS ) {
		error( "cdm module init failed" );
		return 1;
    }

    last_registered_docid = sb_run_server_canneddoc_last_registered_id();
    info("last doc id [%d] from registry", last_registered_docid);

    for(i=1; i<=last_registered_docid; i+= 5000) {
        pid = fork();
        if(pid == 0) { //child
            remake_ac(i, 5000);
            break;
        } else {
            waitpid(pid, &status, 0);
        }
    }

    info("end ac_remake........................");

    return 0;
}

module ac_remake_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	ac_remake_main,			/* child_main */
	NULL,				/* scoreboard */
	NULL,       		/* register hook api */
};

static int del_main(slot_t *slot) {
    uint32_t docid;
    registry_t *reg;
    int last_registered_docid;
    int i, ret;
    lgcaltex_attr_t attr;

    info("start delete doc by SysteName = PTS........................");

    reg = registry_get("LastRegisteredDocId");
    if (reg == NULL) {
        crit("cannot get LastRegisteredDocId registry");
        return 1;
    }

	ret = sb_run_server_canneddoc_init();
	if ( ret != SUCCESS ) {
		error( "cdm module init failed" );
		return 1;
	}

    last_registered_docid = *(int*)reg->data;
    info("last doc id [%d] from registry", last_registered_docid);

    ret = sb_run_docattr_open();
    if (ret == -1) {
        error("cannot initialize docattr module");
        return 1;
    }

    for(i=1; i<=last_registered_docid; i++) {

        if ((i%100) == 0) info("checking SystemName docid[%d]", i);
        else debug("checking SystemName docid[%d]", i);

        if (sb_run_docattr_get(i, &attr) == -1)
        {
            error("cannot get docattr");
            return 1;
        }

        if(attr.is_deleted) continue;

        if(attr.SystemName == 4) {
            docattr_mask_t docmask;
   
            DOCMASK_SET_ZERO(&docmask);
            sb_run_docattr_set_docmask_function(&docmask, "Delete", "1");
            docid = i;
            sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);
        }
    }

    info("end delete doc by SysteName = PTS........................");

    return 0;
}

module del_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	del_main,			/* child_main */
	NULL,				/* scoreboard */
	NULL,       		/* register hook api */
};
