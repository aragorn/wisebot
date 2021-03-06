/* $Id$ */
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <search.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h> /* O_CREAT,O_RDWR */

#include "common_core.h"
#include "common_util.h"
#include "mod_api/docattr.h"
#include "mod_api/qp.h"
#include "mod_api/indexer.h"

#define DOCATTR_ELEMENT_SIZE			docattr_size
static uint32_t max_doc_num = 1000000;
static uint32_t docattr_size = MAX_DOCATTR_ELEMENT_SIZE;
static char docattr_db_file_path[MAX_PATH_LEN] = "dat/cdm/docattr.db";
static int docattr_db_fd = -1;
static void *docattr_array = NULL;
/********************************************************************************/
static int docattr_open() 
{
	struct stat buf;

	char tmp = '\0';
	docattr_db_fd = sb_open(docattr_db_file_path, O_CREAT | O_RDWR, 00666);
	if (docattr_db_fd == -1) {
		error("cannot open docattr db file:%s;%s", 
				docattr_db_file_path, strerror(errno));
		return FALSE;
	}

	if (fstat(docattr_db_fd, &buf) == -1) {
		error("cannot read file stat;%s", strerror(errno));
		return FALSE;
	}

	if (buf.st_size < max_doc_num * DOCATTR_ELEMENT_SIZE) {
		if (lseek(docattr_db_fd, max_doc_num * DOCATTR_ELEMENT_SIZE - 1, 
					SEEK_SET) == (off_t)-1) {
			error("cannot seek end of file[%s];%s", 
					docattr_db_file_path, strerror(errno));
			return FALSE;
		}

		if (write(docattr_db_fd, &tmp, 1) == -1) {
			error("cannot write one dump byte;%s", strerror(errno));
			return FALSE;
		}
	}

	docattr_array = (void*)sb_mmap(NULL, max_doc_num * DOCATTR_ELEMENT_SIZE,
										PROT_READ | PROT_WRITE,
										MAP_SHARED,
										docattr_db_fd, 0);
	if (docattr_array == (void *)MAP_FAILED) {
		error("cannot allocate memory by mmap:%s", strerror(errno));
		return FALSE;
	}

	info("%d bytes is allocated(mapped) by docattr module (shared memory:%p)",
			max_doc_num * DOCATTR_ELEMENT_SIZE, docattr_array);
	return TRUE;
}

// XXX is it need to implement partly synchronizing function?
static int docattr_synchronize() 
{
	int ret;

	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

/*	ret = msync(docattr_array, max_doc_num * sizeof(doc_attr_t), MS_ASYNC);*/
	ret = msync(docattr_array, max_doc_num * DOCATTR_ELEMENT_SIZE, 
			MS_SYNC | MS_INVALIDATE);
	if (ret == -1) {
		error("cannot sync memory to file:error%s", strerror(errno));
		return FALSE;
	}
	return TRUE;
}

static int docattr_close() 
{
	int ret;

	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

	ret = sb_munmap(docattr_array, max_doc_num * DOCATTR_ELEMENT_SIZE);
	if (ret == -1) {
		error("cannot unmap:%s", strerror(errno));
		return FALSE;
	}

	close(docattr_db_fd);

	return TRUE;
}

// FIXME -- ENTRY must be hash table, in the future?
//          temporary coding.... must be modified
static int docattr_get(uint32_t docid, void *p_doc_attr)
{
	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

	memcpy(p_doc_attr, docattr_array+(docid-1)*DOCATTR_ELEMENT_SIZE, 
			DOCATTR_ELEMENT_SIZE);
	
	return TRUE;
}

static int docattr_ptr_get(uint32_t docid, docattr_t **p_doc_attr)
{
	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

	*p_doc_attr = (docattr_t*) (docattr_array+(docid-1)*DOCATTR_ELEMENT_SIZE);

	return TRUE;
}

static int docattr_set(uint32_t docid, void *p_doc_attr)
{
	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

	if ( docid >= max_doc_num ) {
		error("docid[%u] is bigger than max_doc_num[%u]. modify config <mod_docattr.c> - MaxDocNum", docid, max_doc_num);
		return FALSE;
	}

	memcpy(docattr_array+((int)docid-1)*DOCATTR_ELEMENT_SIZE,
			p_doc_attr, DOCATTR_ELEMENT_SIZE);

	return TRUE;
}

static int docattr_get_array(uint32_t *dest_did, int dest_size, uint32_t *src_did, 
		int src_size, condfunc func, void *cond)
{
	int i, j;
	void *ele;

	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

	if (dest_size < src_size)
		warn("dest_size[%d] < src_size[%d], so there can be some missing documents."
			 "dest_size should be larger than src_size.", dest_size, src_size);

	for (i=0, j=0; i<src_size && j<dest_size; i++) {
		ele = docattr_array + (src_did[i]-1) * DOCATTR_ELEMENT_SIZE;

		/* func returns zero when the element should be filtered out,
		 * otherwise func returns non-zero value.
		 */
		if (func(ele, cond, 0)) { /* FIXME ugly hack. please see the type of condfunc. */
			dest_did[j] = src_did[i];
			j++;
		}
	}

	return j;
}

static int docattr_set_array(uint32_t *did_list, int listsize, 
		maskfunc func, void *mask)
{
	int i, ret;
	void *ele;

	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

	for (i=0; i<listsize; i++) {
		if ( did_list[i] >= max_doc_num ) {
			error("docid[%u] is bigger than max_doc_num[%u]. modify config <mod_docattr.c> - MaxDocNum", did_list[i], max_doc_num);
			return FALSE;
		}

		ele = docattr_array + (did_list[i]-1) * DOCATTR_ELEMENT_SIZE;
		if ((ret = func(ele, mask)) < 0) {
			continue;
		}
	}

	return TRUE;
}

static int docattr_get_index_list(sort_base_t *sort_dest, sort_base_t *sort_sour, 
		condfunc func, void *cond)
{
	int i, j, listsize;
	void *ele=NULL;
	int ret;

	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

	if(sort_dest->type == INDEX_LIST) {
		index_list_t* dest = (index_list_t*)sort_dest;
		index_list_t* sour = (index_list_t*)sort_sour;
	    debug("index list->ndochits:%u", dest->ndochits);

	    listsize = sour->ndochits;
		for (i=0, j=0; i<listsize /*&& j<MAX_DOC_HITS_SIZE*/; i++) { 
			// j<MAX_DOC_HITS_SIZE is apparent
			ele = docattr_array + 
				(sour->doc_hits[i].id-1) * DOCATTR_ELEMENT_SIZE;

			/* func returns zero when the element should be filtered out,
			 * otherwise func returns non-zero value.
			 */
			ret = func(ele, cond, sour->doc_hits[i].id);
			if ( ret == MINUS_DECLINE ) return TRUE;
			else if ( ret ) { 
				dest->doc_hits[j] = sour->doc_hits[i];
				j++;
			}
		}
	    dest->ndochits= j;
	} else if(sort_dest->type == AGENT_INFO) {
		agent_light_info_t* dest = (agent_light_info_t*)sort_dest;
		agent_light_info_t* sour = (agent_light_info_t*)sort_sour;
	    debug("agent list->ndochits:%u", dest->recv_cnt);

	    listsize = sour->recv_cnt;
		for (i=0, j=0; i<listsize /*&& j<MAX_DOC_HITS_SIZE*/; i++) { 
			// j<MAX_DOC_HITS_SIZE is apparent
			ele = &(sour->agent_doc_hits[i]->docattr);

			/* func returns zero when the element should be filtered out,
			 * otherwise func returns non-zero value.
			 */
			ret = func(ele, cond, sour->agent_doc_hits[i]->doc_hits.id);
			if ( ret == MINUS_DECLINE ) return TRUE;
			else if ( ret ) { 
				dest->agent_doc_hits[j]->doc_hits = sour->agent_doc_hits[i]->doc_hits;
				j++;
			}
		}
	    dest->recv_cnt= j;
	}

	// FIXME
	// no need to set members of index_list_t? 
	// such as type, prev, nex, header, op_not, nhits?
	return TRUE;
}

// FIXME ???
static int docattr_set_index_list(index_list_t *list, maskfunc func, void *mask)
{
	int i, listsize, ret;
	void *ele=NULL;

	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

	listsize = list->ndochits;
	for (i=0; i<listsize; i++) {
		ele = docattr_array + 
			(list->doc_hits[i].id-1) * DOCATTR_ELEMENT_SIZE;
		if ((ret = func(ele, mask)) < 0) {
			continue;
		}
	}

	return TRUE;
}

static int docattr_index_list_sortby(sort_base_t *sort_base, void *userdata,
		int (*compar)(const void *, const void *, void *))
{
	if (docattr_array == NULL && docattr_open() != TRUE) return FALSE;

	if(sort_base->type == INDEX_LIST) {
		index_list_t* list = (index_list_t*)sort_base;
	    debug("index list->ndochits:%u", list->ndochits);
	    qsort2(list->doc_hits, list->ndochits, sizeof(doc_hit_t), userdata, compar);
	} else if(sort_base->type == AGENT_INFO) {
		agent_light_info_t* info = (agent_light_info_t*)sort_base;
	    debug("agent list->ndochits:%u", info->recv_cnt);
	    qsort2(info->agent_doc_hits, info->recv_cnt, sizeof(agent_doc_hits_t*), userdata, compar);
	}
	return TRUE;
}

/******************************************************************************/
static void get_docattr_db_file_path(configValue v) {
	strncpy(docattr_db_file_path, v.argument[0], 256);
	docattr_db_file_path[255] = '\0';
}

static void get_max_doc_num(configValue v) {
	max_doc_num = atoi(v.argument[0]);
	info("max_doc_num is setted %d", max_doc_num);
}

static void get_docattr_size(configValue v) {
	docattr_size = atoi(v.argument[0]);
	if (docattr_size > MAX_DOCATTR_ELEMENT_SIZE) {
		crit("DocAttrSize(%d) is bigger than MAX_DOCATTR_ELEMENT_SIZE(%d)",
				docattr_size, MAX_DOCATTR_ELEMENT_SIZE);
		docattr_size = MAX_DOCATTR_ELEMENT_SIZE;
	}
}

static config_t config[] = {
	CONFIG_GET("MaxDocNum", get_max_doc_num, 1, "document maximun number"),
	CONFIG_GET("DocAttrSize", get_docattr_size, 1, "document data size"),
	CONFIG_GET("DocattrDBFilePath", get_docattr_db_file_path, 1, "docattr db file path"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_docattr_open(docattr_open,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_close(docattr_close,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_synchronize(docattr_synchronize,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_get(docattr_get,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_ptr_get(docattr_ptr_get,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_set(docattr_set,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_get_array(docattr_get_array,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_set_array(docattr_set_array,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_get_index_list(docattr_get_index_list,
			NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_set_index_list(docattr_set_index_list,
			NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_index_list_sortby(docattr_index_list_sortby,
			NULL,NULL,HOOK_MIDDLE);
}

module docattr_module = {
	STANDARD_MODULE_STUFF,
	config,					/* conf table */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
