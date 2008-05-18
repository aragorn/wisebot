/* $Id$ */
#include "softbot.h"
#include "mod_api/docattr_new.h"
#include "mod_qp/mod_qp.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <search.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct _docattr_db_info_t {
	int file_num;
} docattr_db_info_t;

struct _docattr_db_t { 
   char docattr_db_file_path[MAX_PATH_LEN]; /* database file path */ 
   int docattr_size;                        /* docattr 의 단위 사이즈: 
                                               따로 hooking되는 모듈들에서 정의된 
                                               구조체의 사이즈와 일치해야 한다. */ 
   docattr_db_info_t *db_info;  			/* last number of database file */ 
   void *docattr_arrays[MAX_FILE_NUM]; 		/* 할당된(shm) 메모리 테이블 */ 
}; 


static int load_docattr_db_info(docattr_db_t *docattr_db)
{
	int fd;
	struct stat buf;
	char tmp = '\0';
    char file_path[MAX_PATH_LEN];

	sprintf(file_path, "%s.info", docattr_db->docattr_db_file_path);

	fd = sb_open(file_path, O_CREAT | O_RDWR, 00666);
	if (fd == -1) {
		error("cannot open docattr db info file:%s;%s", 
				file_path, strerror(errno));
		return FAIL;
	}

	if (wr_lock(fd, SEEK_SET, 0, 0) == -1) {
		error("cannot get a lock on docattr file[%s]: %s", 
				docattr_db->docattr_db_file_path, 
				strerror(errno));
		return FAIL;
	}
	if (fstat(fd, &buf) == -1) {
		error("cannot read file stat;%s", strerror(errno));
		un_lock(fd, SEEK_SET, 0, 0);
		return FAIL;
	}

    if (buf.st_size < sizeof(docattr_db_info_t)) {
        if (lseek(fd, sizeof(docattr_db_info_t) - 1, SEEK_SET) == (off_t)-1) {
            error("cannot seek end of file[%s];%s", 
					file_path, strerror(errno));
            return FAIL;
        }

        if (write(fd, &tmp, 1) == -1) {
            error("cannot write one dump byte;%s", strerror(errno));
            return FAIL;
        }
    } else {
		info("docattr_db info exist");
	}
	
	info("^^^ using mmap  ########");
	docattr_db->db_info = (docattr_db_info_t *)sb_mmap(NULL, sizeof(docattr_db_info_t), 
							PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (docattr_db->db_info == NULL) {
		error("docattr_db->db_info mmap return NULL");	
		return FAIL;
	}


	close(fd);
	return SUCCESS;
}

static int get_docattr_file_path(char *dest, char *path, int file_idx)
{
	return sprintf(dest, "%s.%03d", path, file_idx);
}

static int create_docattr_file(docattr_db_t *docattr_db, int file_idx)
{
	int fd;
	struct stat buf;
	char file_path[MAX_PATH_LEN];
	char tmp ='\0';
	
	if (MAX_FILE_NUM <= file_idx) {
		error("MAX_FILE_NUM[%d] <= file_idx[%d]", MAX_FILE_NUM, file_idx); 	
		return FAIL;
	}
	
	get_docattr_file_path(file_path, docattr_db->docattr_db_file_path, file_idx);	

	fd = sb_open(file_path, O_CREAT | O_RDWR, 00666);
	if (fd == -1) {
		error("cannot open docattr file:%s;%s", 
				file_path, strerror(errno));
		return FAIL;
	}
	if (fstat(fd, &buf) == -1) {
		error("cannot read file stat:%s", strerror(errno));
		return FAIL;
	}

    if (buf.st_size < DOCATTR_FILE_SIZE) {
        if (lseek(fd, DOCATTR_FILE_SIZE, SEEK_SET) == (off_t)-1) {
            error("cannot seek end of file[%s];%s", file_path, strerror(errno));
            return FAIL;
        }

        if (write(fd, &tmp, 1) == -1) {
            error("cannot write one dump byte;%s", strerror(errno));
            return FAIL;
        }
    }
	
	info("^^^ using mmap  ########");
	docattr_db->docattr_arrays[file_idx] = 
		(void *)sb_mmap(NULL, DOCATTR_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (docattr_db->docattr_arrays[file_idx] == NULL) return FAIL;

	info("docattr_db->docattr_arrays[file_idx] = %p size is %d ", 
		  docattr_db->docattr_arrays[file_idx], DOCATTR_FILE_SIZE);

	close(fd);	
	return SUCCESS;
}

static int load_docattr_file(docattr_db_t *docattr_db, int file_idx)
{
	int fd;
	struct stat buf;
	char file_path[MAX_PATH_LEN];
	
	get_docattr_file_path(file_path, docattr_db->docattr_db_file_path, file_idx);	

	fd = sb_open(file_path, O_CREAT | O_RDWR, 00666);
	if (fd == -1) {
		error("cannot open docattr file:%s;%s", 
				file_path, strerror(errno));
		return FAIL;
	}
	if (fstat(fd, &buf) == -1) {
		error("cannot read file stat;%s", strerror(errno));
		return FAIL;
	}

	if (buf.st_size != DOCATTR_FILE_SIZE + 1 ) {
		error("file [%s] : size [%d] != DOCATTR_FILE_SIZE [%d]",
				file_path, (int)buf.st_size, DOCATTR_FILE_SIZE);
		return FAIL;
	}
	
	info("^^^ using mmap  ########");
	docattr_db->docattr_arrays[file_idx] = 
		(void *)sb_mmap(NULL, DOCATTR_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (docattr_db->docattr_arrays[file_idx] == NULL) return FAIL;

	close(fd);
	return SUCCESS;
}

static int load_docattr_table(docattr_db_t *docattr_db)
{
	int i;

	for (i=0; i<docattr_db->db_info->file_num; i++) {
		if (load_docattr_file(docattr_db, i) != SUCCESS) return FAIL;
	}

	debug("load [%d] docattr_db file", docattr_db->db_info->file_num);
	return SUCCESS;
}


/********************************************************************************/
static docattr_db_t *docattr_open(char path[], int docattr_size) 
{
	docattr_db_t *docattr_db;

	docattr_db = (docattr_db_t *)sb_malloc(sizeof(docattr_db_t));
	if (docattr_db == NULL) return NULL;
	memset(docattr_db, 0, sizeof(docattr_db_t));
	
	strncpy(docattr_db->docattr_db_file_path, path, MAX_PATH_LEN);
	docattr_db->docattr_size = docattr_size;

	if (load_docattr_db_info(docattr_db) != SUCCESS) return NULL;
	debug(" docattr_db file num is [%d]", docattr_db->db_info->file_num);

	if (load_docattr_table(docattr_db) != SUCCESS) return NULL;

	return docattr_db;
}


// XXX is it need to implement partly synchronizing function?
static int docattr_synchronize(docattr_db_t *docattr_db) 
{
	int i, ret;

	for (i=0 ; i<docattr_db->db_info->file_num ; i++) {
		ret = msync(docattr_db->docattr_arrays[i],
					DOCATTR_FILE_SIZE, MS_SYNC | MS_INVALIDATE);
		if (ret == -1) {
			error("cannot sync memory to file:error%s", strerror(errno));
			return FAIL;
		}
	}

	ret = msync((void *)docattr_db->db_info, sizeof(docattr_db_info_t),
			MS_SYNC | MS_INVALIDATE);
	if (ret == -1) {
		error("cannot sync docattr_db : %s",strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}

static int docattr_close(docattr_db_t *docattr_db) 
{
	int i, ret;
	
	for (i=0 ; i<docattr_db->db_info->file_num ; i++) {
		info("~~~~ munmapping ");
		ret = sb_munmap(docattr_db->docattr_arrays[i], DOCATTR_FILE_SIZE);
		if (ret == -1) {
			error("cannot sync memory to file:error%s", strerror(errno));
			return FAIL;
		}
	}

	info("~~~~ munmapping ");
	ret = sb_munmap((void *)docattr_db->db_info, sizeof(docattr_db_info_t));
	if (ret == -1) {
		error("cannot unmap docattr_db :%s", strerror(errno));
		return FAIL;
	}

	sb_free(docattr_db);
	return SUCCESS;
}

static int docattr_get(docattr_db_t *docattr_db, uint32_t docid, void *p_doc_attr)
{
	int file_idx;
	int remind;

	file_idx = (docid-1) / DOCATTR_NUM_IN_ONE_FILE;
	remind =   (docid-1) % DOCATTR_NUM_IN_ONE_FILE;

	if (docattr_db->db_info->file_num <= file_idx) {
		error(" file_num [%d] <= file_idx[%d] : docid-1 = [%u]",
				docattr_db->db_info->file_num, file_idx, docid-1);
		return FAIL;
	}

	memcpy(p_doc_attr, 
		   docattr_db->docattr_arrays[file_idx] + (remind * docattr_db->docattr_size),
		   docattr_db->docattr_size);
	
	return SUCCESS;
}

static int docattr_set(docattr_db_t *docattr_db, uint32_t docid, void *p_doc_attr)
{
	int file_idx;
	int remind;

	file_idx = (docid-1) / DOCATTR_NUM_IN_ONE_FILE;
	remind   = (docid-1) % DOCATTR_NUM_IN_ONE_FILE;

	if (docattr_db->db_info->file_num == file_idx) {
		info("creat docattr file [%d]",file_idx);
		if (create_docattr_file(docattr_db, file_idx) != SUCCESS) return FAIL;
		docattr_db->db_info->file_num++;
	} else if (docattr_db->db_info->file_num < file_idx) {
		error(" file_num [%d] < file_idx[%d] : docid-1 = [%u]",
				docattr_db->db_info->file_num, file_idx, docid-1);
		return FAIL;
	}

	memcpy(((void *)docattr_db->docattr_arrays[file_idx]) + (remind * docattr_db->docattr_size),
		   p_doc_attr, docattr_db->docattr_size);
	
	return SUCCESS;
}

#if 0
static int docattr_get_array(DocId *dest, int destsize, DocId *sour, 
		int soursize, condfunc func, void *cond)
{
	int i, j;
	void *ele;

	for (i=0, j=0; i<soursize && j<destsize; i++) {
		ele = docattr_array + (sour[i]-1) * DOCATTR_ELEMENT_SIZE;

		/* func returns zero when the element should be filtered out,
		 * otherwise func returns non-zero value.
		 */
		if (func(ele, cond)) {
			dest[j] = sour[i];
			j++;
		}
	}

	return j;
}
#endif

static int docattr_set_array(docattr_db_t *docattr_db, uint32_t list[], 
		int list_size, void *mask)
{
	int file_idx, remind;
	int i, ret;
	void *ele;

	for (i=0; i<list_size; i++) {
		
		file_idx = (list[i]-1) / DOCATTR_NUM_IN_ONE_FILE;
		remind =   (list[i]-1) % DOCATTR_NUM_IN_ONE_FILE;
		
		if (docattr_db->db_info->file_num == file_idx) {
			info("creat docattr file [%d]",file_idx);
			if (create_docattr_file(docattr_db, file_idx) != SUCCESS) return FAIL;
			docattr_db->db_info->file_num++;
		}
		else if (docattr_db->db_info->file_num < file_idx) {
			error(" file_num [%d] < file_idx[%d] : list[%d]-1 = [%u]",
					docattr_db->db_info->file_num, file_idx, i, list[i]-1);
			return FAIL;
		}

		ele = docattr_db->docattr_arrays[file_idx] + (remind * docattr_db->docattr_size);
		
		if ((ret = sb_run_docattr_mask_function(ele, mask)) < 0) {
			continue;
		}
	}

	return SUCCESS;
}


static int docattr_get_index_list(docattr_db_t *docattr_db, 
		struct index_list_t *dest, struct index_list_t *sour, void *cond)
{
	int file_idx, remind;
	int i, j, listsize;
	void *ele=NULL;

	listsize = sour->ndochits;
	for (i=0, j=0; i<listsize /*&& j<MAX_DOC_HITS_SIZE*/; i++) { 
											// j<MAX_DOC_HITS_SIZE is apparent
		
		file_idx = (sour->doc_hits[i].id-1) / DOCATTR_NUM_IN_ONE_FILE;
		remind =   (sour->doc_hits[i].id-1) % DOCATTR_NUM_IN_ONE_FILE;
		
		if (docattr_db->db_info->file_num <= file_idx) {
			error(" file_num [%d] <= file_idx[%d] : sour->doc_hits[%d].id-1 = [%u]",
					docattr_db->db_info->file_num, file_idx, i, sour->doc_hits[i].id-1);
			return FAIL;
		}
	
		ele = docattr_db->docattr_arrays[file_idx] + (remind * docattr_db->docattr_size);
		
		/* func returns zero when the element should be filtered out,
		 * otherwise func returns non-zero value.
		 */
		if (sb_run_docattr_compare_function(ele, cond)) {
			dest->doc_hits[j] = sour->doc_hits[i];
			j++;
		}
	}
	dest->ndochits= j;
	// FIXME
	// no need to set members of index_list_t? 
	// such as type, prev, nex, header, op_not, nhits?
	return TRUE;
}

#if 0 
// FIXME ???
static int docattr_set_index_list(index_list_t *list, maskfunc func, void *mask)
{
	int i, listsize, ret;
	void *ele=NULL;

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
#endif


static int docattr_index_list_sortby(docattr_db_t *docattr_db, 
		struct index_list_t *list, void *userdata,
		int (*compar)(const void *, const void *, void *))
{
//	CRIT("before qsort2");
//	CRIT("list:%p",list);
	debug("list->ndochits:%u", list->ndochits);
	qsort2(list->doc_hits, list->ndochits, sizeof(doc_hit_t), userdata, compar);
//	CRIT("after qsort2");
	return TRUE;
}

/******************************************************************************/
#if 0
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
#endif

static void register_hooks(void)
{
	sb_hook_docattr_open(docattr_open,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_close(docattr_close,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_synchronize(docattr_synchronize,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_get(docattr_get,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_set(docattr_set,NULL,NULL,HOOK_MIDDLE);
/*	sb_hook_docattr_get_array(docattr_get_array,NULL,NULL,HOOK_MIDDLE);*/
	sb_hook_docattr_set_array(docattr_set_array,NULL,NULL,HOOK_MIDDLE);
	sb_hook_docattr_get_index_list(docattr_get_index_list,
			NULL,NULL,HOOK_MIDDLE);
/*	sb_hook_docattr_set_index_list(docattr_set_index_list,*/
/*			NULL,NULL,HOOK_MIDDLE);*/
	sb_hook_docattr_index_list_sortby(docattr_index_list_sortby,
			NULL,NULL,HOOK_MIDDLE);
}

module docattr_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* conf table */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
