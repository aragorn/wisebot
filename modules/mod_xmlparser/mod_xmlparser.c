/* $Id$ */
#include "softbot.h"
#include "mod_api/xmlparser.h"

static void register_hooks(void)
{
//	sb_hook_xmlparser_parse(parse,NULL,NULL,HOOK_MIDDLE);
	sb_hook_xmlparser_parselen(parselen,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_parser_create(parser_create,NULL,NULL,HOOK_MIDDLE);
	sb_hook_xmlparser_free_parser(free_parser,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_loaddom(loaddom,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_loaddom2(loaddom2,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_savedom(savedom,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_savedom2(savedom2,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_get_domsize(get_domsize,NULL,NULL,HOOK_MIDDLE);
	sb_hook_xmlparser_retrieve_field(retrieve_field,NULL,NULL,HOOK_MIDDLE);
//	sb_hook_xmlparser_retrieve_attr(retrieve_attr,NULL,NULL,HOOK_MIDDLE);
}

module xmlparser_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,               /* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks		/* register hook api */
};
