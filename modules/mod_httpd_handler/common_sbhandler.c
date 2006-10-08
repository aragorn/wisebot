/* $Id$ */
#include "common_core.h"
#include "mod_standard_handler.h"
#include "mod_httpd/protocol.h"
#include "mod_api/sbhandler.h"
#include <stdlib.h>

static int view_status_handler(request_rec *r, softbot_handler_rec *s);

static softbot_handler_key_t common_handler_tbl[] =
{	
	{"view_status", view_status_handler},
	{NULL, NULL}
};

/////////////////////////////////////////////
#define SBINTERNET_TMP_DIR   "tmp"
int sb_mktemp(char *tmp_file){
    //TODO : get temp directory
    memset(tmp_file, 0, MAX_FILE_LEN);
    snprintf(tmp_file, MAX_FILE_LEN, "%s/XXXXXX", SBINTERNET_TMP_DIR);
    if ( mktemp(tmp_file) == NULL ) {
        error("get temp file name failed");
        return FAIL;
    }

    return SUCCESS;
}

static int view_status_handler(request_rec *r, softbot_handler_rec *s){
	char file[MAX_FILE_LEN];
	FILE *fp;
	char *target;
	char *module_name;

	CHECK_REQUEST_CONTENT_TYPE(r, NULL);
	
	module_name = (char *)apr_table_get(s->parameters_in, "module");

	if ( s->remain_uri == NULL || s->remain_uri[0] != '/'){
		error("null remain_uri");
		return FAIL;
	}
	target = s->remain_uri+1;
	if (sb_mktemp(file) != SUCCESS ) {
		error("sb_mktemp failed");
		return FAIL;
	}
	fp = sb_fopen(file, "wb");
	if ( !fp){
		error("sb_fopen [%s] failed : %s", file, strerror(errno));
		return FAIL;
	}

	if (strncmp(target, "modules", 7) == 0 ){
		list_modules(fp);
	}else if (strncmp(target, "static_modules", 14) == 0 ){
		list_static_modules(fp);
	}else if (strncmp(target, "config", 6) == 0 ){
		list_config(fp, module_name);
	}else if (strncmp(target, "registry", 8) == 0 ){
		list_registry(fp, module_name);
	}else if (strncmp(target, "scoreboard", 10) == 0 ){
		list_scoreboard_xml(fp, module_name);
	}/*else if (strncmp(target, "information", 11) == 0 ){
		int nRet;
		char *info = (char *)apr_table_get(s->parameters_in, "info");
		
		if ( module_name == NULL ){
			module *m = NULL;
			//FIXME very dirty coding
			long tmp_offset = 0;
			fprintf(fp, "<?xml version=\"1.0\" encoding=\"euc-kr\" ?><xml>"
					"<!-- Information about all modules -->\n" );
			for(m=first_module; m; m=m->next ) {
				tmp_offset = ftell(fp);
				fprintf(fp, "<module name=\"%s\" >\n", m->name);
				nRet = sb_run_sbmgr_list_info(fp, (char *)m->name, NULL);
				if ( nRet != SUCCESS ) {
					if ( nRet == DECLINE )	{
						fseek(fp, tmp_offset, SEEK_SET);
						ftruncate(fileno(fp), (off_t) tmp_offset );
					}else {
						 error("sb_run_sbmgr_list_info failed");
						fclose(fp);
						sb_unlink(file);
						return FAIL;
					}
				}else {
					fprintf(fp, "</module>\n");
				}
			}
			fprintf(fp, "</xml>\n");
		}else {
			fprintf(fp, "<?xml version=\"1.0\" encoding=\"euc-kr\" ?><xml>"
					"<!-- Information about module[%s] -->\n"
					"<module name=\"%s\" >\n", module_name, module_name);
			nRet = sb_run_sbmgr_list_info(fp, module_name, info);
			if ( nRet != SUCCESS ) {
				if ( nRet == DECLINE )	{
					error("module [%s] does not have information %s", module_name, (info) ? info : "." );
				}else	error("sb_run_sbmgr_list_info failed");
				fclose(fp);
				sb_unlink(file);
				return FAIL;
			}
			fprintf(fp, "</module>\n</xml>");
		}
	}*/else {
		error("[common/%s/%s] not implemeted", s->request_name, target);
		fclose(fp);
		sb_unlink(file);
		return FAIL;
	}
	fclose(fp);

	ap_set_content_type(r, "text/xml; charset=euc-kr");
	if( sb_run_sbhandler_append_file(r, file) != SUCCESS ) {
		error("sb_run_sbhandler_append_file failed");
		sb_unlink(file);
		return FAIL;
	}
	sb_unlink(file);

	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////

int sbhandler_common_get_table(char *name_space, void **tab)
{
	if ( strcmp(name_space, "common") != 0 ){
		return DECLINE;
	}

	*tab = common_handler_tbl;

	return SUCCESS;
}
