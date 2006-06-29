/* $Id$ */
#include "common_core.h"
#include <string.h> // strcpy
#include "mod_api/xmlparser.h"
#include "parser.h"

static int init_unicode_name = 0;

static int init()
{
	if ( !init_unicode_name ) {
#if defined(AIX5) || defined(SOLARIS) || defined(HPUX)
		strcpy( unicode_name, "UTF-16LE" );
#else
		strcpy( unicode_name, "UNICODE" );
#endif
	}
	return SUCCESS;
}

static void get_unicode_name(configValue v)
{
	strncpy( unicode_name, v.argument[0], sizeof(unicode_name) );
	init_unicode_name = 1;
}

static config_t config[] = {
	CONFIG_GET("UnicodeName", get_unicode_name, 1, "cp949 -> unicode"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_xmlparser_parselen(parselen,NULL,NULL,HOOK_MIDDLE);
	sb_hook_xmlparser_retrieve_field(retrieve_field,NULL,NULL,HOOK_MIDDLE);
	sb_hook_xmlparser_free_parser(free_parser,NULL,NULL,HOOK_MIDDLE);

//	sb_hook_xmlparser_parse(parse,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_parser_create(parser_create,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_loaddom(loaddom,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_loaddom2(loaddom2,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_savedom(savedom,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_savedom2(savedom2,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_get_domsize(get_domsize,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_retrieve_attr(retrieve_attr,NULL,NULL,HOOK_MIDDLE);
}

module xmlparser_module = {
	STANDARD_MODULE_STUFF,
	config,				/* config */
	NULL,				/* registry */
	init,               /* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks		/* register hook api */
};
