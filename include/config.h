/* $Id$ */
#ifndef CONFIG_H
#define CONFIG_H 1

#include "constants.h"

#define MAX_CONFIG_ARG_NUM      (32)
#define MAX_CONFIG_LINE         (256)
#define DYNAMIC_MODULE_LIMIT    (20)
#define MODULE_CONFIG_LIMIT     (100)

#define VAR_ARG	(-1)

//typedef char string[STRING_SIZE]; XXX: obsolete

typedef struct {
    char argument[MAX_CONFIG_ARG_NUM][STRING_SIZE];
	int  argNum;
} configValue;

typedef struct {
    const char *tag;              /* tag name = module name */
    const char *name;             /* name of config */
    void (*set)();                /* callback function to set value */
    const int  argNum;            /* number of arguments */
    const char *desc;             /* description of config */
} config_t;

extern int check_config_syntax;

//void set_cmdline_arg(char *key,char *value);

#define CONFIG_GET(name, set, argNum, desc) \
		{__FILE__,name, set, argNum, desc}

#endif
