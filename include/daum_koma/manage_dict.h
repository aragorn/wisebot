#ifndef _MANAGE_DICT_H
#define _MANAGE_DICT_H 1

#include "moran.h"

// XXX: temporary
#define MAX_PATHSTRING_SIZE			(512)
#define MAX_DIC_REGISTRY_SIZE		(16)

typedef struct _dic_t dic_t;

// XXX: temporary
typedef struct _dic_manager_t dict_manager_t;

typedef struct _dic_manager_t dic_manager_t;

struct _dic_manager_t {
	// XXX: temporary
	char *sdict;
	char *udict;
	char *xdict;
	char *predict;
	char *alias;
	char *space;
	char *fdict;

	// private
	int dicnum;
	dic_t *dic_registry[MAX_DIC_REGISTRY_SIZE];
};

typedef int (*dic_init_func)(dic_t *dic);
typedef int (*dic_open_func)(dic_t *dic);
typedef void (*dic_close_func)(dic_t *dic);
typedef int (*dic_search_func)(dic_t *dic, const char *key, void *outputbuf, int bufsize);
typedef int (*dic_insert_func)(dic_t *dic, const char *key, void *inputdata, int datasize);
typedef int (*dic_update_func)(dic_t *dic, const char *key, void *updatedata, int datasize);
typedef int (*dic_delete_func)(dic_t *dic, const char *key);

struct _dic_t {
	// public
	char path[MAX_PATHSTRING_SIZE];
	int use; // YES or NO

	// private
	unsigned char *data;
	int datasize;
	void *childobj;

	// XXX: should not be here!!
	int first_char_addr[MAX_DICT_FIRSTS];
	int tags_count;
	TAG_INFO *ptag_info;

	// handlers
	dic_init_func init;
	dic_open_func open;
	dic_close_func close;
	dic_search_func search;
	dic_insert_func insert;
};

// XXX: these two functions are legacy!!
int HANL_open_dicts(dict_manager_t *dict, const char configfile[]);
int HANL_close_dicts();

dic_t *dicmanager_create_dic_handler(const char path[],
									 dic_init_func init,
									 dic_open_func open,
									 dic_close_func close,
									 dic_search_func search,
									 dic_insert_func insert);
void dicmanager_destroy_dic_handler(dic_t *dic);

int dicmanager_open_dics(dic_manager_t *manager);
void dicmanager_close_dics(dic_manager_t *manager);
#endif
