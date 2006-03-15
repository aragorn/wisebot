/* $Id$ */
#ifndef CONFIG_H
#define CONFIG_H 1

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

#define MAX_CONFIG_ARG_NUM      (32)
#define MAX_CONFIG_LINE         (256)
#define DYNAMIC_MODULE_LIMIT    (20)
#define MODULE_CONFIG_LIMIT     (100)

#define VAR_ARG	(-1)

typedef struct {
    char argument[MAX_CONFIG_ARG_NUM][STRING_SIZE];
	int  argNum;
} configValue;

struct config_t {
    const char *tag;              /* tag name = module name */
    const char *name;             /* name of config */
    void (*set)();                /* callback function to set value */
    const int  argNum;            /* number of arguments */
    const char *desc;             /* description of config */
};

extern int check_config_syntax;
SB_DECLARE(int) read_config(char *configPath, module *start_module);

#define CONFIG_GET(name, set, argNum, desc) \
		{__FILE__,name, set, argNum, desc}

#endif
