/* $Id$ */
#ifndef REGISTRY_H
#define REGISTRY_H 1

#define MAX_EACH_REGISTRY_RESULT_STRING_SIZE (128)
#define REGISTRY static

enum save_flag {
	SAVE,
	DONT_SAVE
};

#define PERSISTENT_REGISTRY(name, desc, size, init, get, set)\
		{__FILE__, name, desc, NULL, NULL, size, init, get, set, SAVE }
#define RUNTIME_REGISTRY(name, desc, size, init, get, set)\
		{__FILE__, name, desc, NULL, NULL, size, init, get, set, DONT_SAVE}
#define NULL_REGISTRY \
		{NULL,NULL,NULL,NULL,NULL,0,NULL,NULL,NULL,DONT_SAVE}

typedef struct {
    char *module;			/* module name */
    char *name;           	/* name of registry */
    char *desc;           	/* description of registry */
	void *data;				/* pointer of data value */
	void *data_relative;	/* relative pointer from shared memory starting */
	int size;				/* size of data value */
	void (*init)(void*);	/* linking registry data pointer to shared memory */
    char* (*get)();			/* callback function to get value */
    char* (*set)(void*);	/* callback function to set value */
							/* FIXME: is set function needed to return char*? */
	enum save_flag flag;	/* flag to save registry or not */
} registry_t;

SB_DECLARE(void) load_each_registry();
SB_DECLARE(void) list_registry(FILE *out, char *module_name);
SB_DECLARE(void) list_registry_str(char *result, char *module_name);
SB_DECLARE(int) save_registry_file(char *file);
SB_DECLARE(int) save_registry(FILE *out, char *module_name);
SB_DECLARE(int) save_registry_str(char *result, char *module_name);
SB_DECLARE(int) restore_registry_file(char *file);
SB_DECLARE(int) restore_registry(FILE *in, char *module_name);
SB_DECLARE(registry_t*) registry_get(char *registry_name);

#endif
