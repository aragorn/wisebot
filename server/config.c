/* $Id$ */
#include "softbot.h"

#define DEFAULT_TAG "global"

#define INCLUDE_DIRECTIVE "Include" 
//#define ADDMODULE_DIRECTIVE "AddModule"

#define FORMAT  "| %-15s | %-10s | %-3d | %-15s | %-30s |\n"

int check_config_syntax = 0;

/* function prototype */
static int  lineParse(const char line[], char (*argument)[STRING_SIZE]);
static void getConfig(const char line[], int *i ,char argument[]);
static void skipWhiteSpace(const char line[], int *i);
static int  isTag(char ch);
static int  isComment(char ch);
static int  isWhiteSpace(char ch);

static int  makeTagState(char line[]);
static int  startTag(char line[]);
static int  endTag(char line[]);
static int  findCharacter(char line[] , char ch);

static int  conf_process(const char line[], module *start_module);

static void copyString(char *dest, const char *src);
static int isEqualString(const char *a,const char *b);

/* declare variable */
/*static int  loadModule(char symbol[], char path[]);*/
/*static void *handle[DYNAMIC_MODULE_LIMIT];*/
/*static int  dynamicModuleNum = 0;*/

static char configTag [STRING_SIZE] = DEFAULT_TAG;

/* XXX: of no use now, 
 *      but can be changed to overwrite registry --jiwon
 */
#if 0
#define MAX_CMDLINE_CONFIG	20
static char cmdline_args[2][MAX_CMDLINE_CONFIG][STRING_SIZE];
static int cmdline_config_num=0;
int read_cmdline_arg()
{
	module *m=NULL;
	int i=0,j=0;
	configValue c;
	c.argNum=1;	/* commandline argument의 경우는 argument가 항상 하나임. */

	if (check_config_syntax) {
		info("Commandline configuration:");
		for (j=0; j<cmdline_config_num; j++) {
			info("[%s]: [%s]",cmdline_args[KEY][j],cmdline_args[VALUE][j]);
		}
	}
	
	for (m=first_module; m; m=m->next) {
		if (mod->config == NULL) {
			continue;
		}
		for (i=0; mod->config[i].name!=NULL; i++) {
			for (j=0; j<cmdline_config_num; j++) {
				if (strncmp(mod->config[i].name,
						cmdline_args[KEY][j],STRING_SIZE)==0) {
					strncpy(c.argument[0],cmdline_args[VALUE][j],STRING_SIZE);
					c.argument[0][STRING_SIZE-1]='\0';
					break;
				}
			}
		}
	}
}
void set_cmdline_arg(char *key, char *value)
{
	if (cmdline_config_num == MAX_CMDLINE_CONFIG) {
		warn("too many commandline config. [%s](%s) is not set",
				key,value);
		return;
	}
	strncpy(cmdline_args[KEY][cmdline_config_num],key,STRING_SIZE);
	strncpy(cmdline_args[VALUE][cmdline_config_num],value,STRING_SIZE);
	cmdline_config_num++;
}
#endif

/* read configuration one line and run callback */
int read_config(char *path, module *start_module)
{
 	FILE *fp;
	char buffer[STRING_SIZE];
	int nLen, i, configLineNum=0;

	/* file open */
	if ((fp=sb_fopen(path, "r"))==NULL) {
		error("can't open config file \"%s\"", path);
		return FAIL;
	}

	/* file read line by line */ 
	while (1) {
		if (fgets(buffer, STRING_SIZE, fp) == NULL && feof(fp)) {
			info("reading configuration file[%s] done",path);
			break;
		}
		nLen = strlen(buffer);
		i = 0; 
		
		// skip front space 
		skipWhiteSpace(buffer , &i);
		if (nLen < i)
			continue;
	
		if (isComment(buffer[i]) 	== TRUE) {
			// cmment doing nothing
		} else if (isTag(buffer[i]) == TRUE) {
			if (makeTagState(buffer) != SUCCESS) {
				error("makeTagState error");
				return FAIL;
			}
		} else {
			if (start_module == NULL) start_module = first_module;
			if (conf_process(buffer, start_module) != SUCCESS) {
				error("conf_process error");
				return FAIL;
			}
			configLineNum++;
		}
	}

	return SUCCESS;
}

/* line process */
static int conf_process(const char line[], module *start_module)
{
	char argument[MAX_CONFIG_ARG_NUM][STRING_SIZE];
	int i=0, argNum=0, called = FALSE;
	configValue a;
	module* mod=NULL;

	for(i=0;i<MAX_CONFIG_ARG_NUM;i++)
		memset(argument[i], 0x00, STRING_SIZE);

	// parse line
	argNum = lineParse(line , argument);

	if (check_config_syntax) {
		char buf[STRING_SIZE]="";
		snprintf(buf,STRING_SIZE,"[%s]: ", argument[0]);

		for (i=1; i<argNum; i++) {
			snprintf(buf+strlen(buf), STRING_SIZE, "[%s] ", argument[i]);
		}

		info("%s",buf);
	}

	if (isEqualString(argument[0], INCLUDE_DIRECTIVE) == TRUE) {
		notice("reading configuration from [%s]",argument[1]);
		read_config(argument[1], start_module);	
	} else {
		// copy argument
		for (i=0 ; i < argNum - 1; i++) {
			copyString(a.argument[i] , argument[i+1] );
		}

		// argument number return
		a.argNum = argNum-1; 
	
		// find equal name, tag  and run call back function
		// call back function	
		for (mod=start_module; mod; mod=mod->next){	
			if (mod->config == NULL){
				continue;
			}
			for (i=0 ; mod->config[i].name!=NULL ; i++) {
				if (strrchr(mod->config[i].tag,'/'))
        			mod->config[i].tag = 1+ strrchr(mod->config[i].tag,'/');
    			if (strrchr(mod->config[i].tag,'\\'))
        			mod->config[i].tag = 1+ strrchr(mod->config[i].tag,'\\');
				
				if ( ((isEqualString(mod->config[i].tag, configTag) == TRUE) || 
					 (isEqualString(configTag, DEFAULT_TAG) == TRUE)) &&
				    (isEqualString(mod->config[i].name, argument[0]) == TRUE) ){
					
					if (mod->config[i].argNum != VAR_ARG &&
							a.argNum != mod->config[i].argNum) {
						warn("%s:[%s] number of argument mismatch",
							  mod->name,  mod->config[i].name);
						warn("expecting %d but got %d argument (%s)",
								mod->config[i].argNum,a.argNum, line);
						warn("config(%s,%s,%p,%d,%s)",
								mod->config[i].tag,
								mod->config[i].name,
								mod->config[i].set,
								mod->config[i].argNum,
								mod->config[i].desc
						);
					}
					
					// call back function	
					(*mod->config[i].set)(a);
					called = TRUE;
				}
			}//for each callback function of a module
		}// for each module
		if (called == FALSE)
			warn(" %s - %s is not called",configTag ,argument[0]); 
	}

	return SUCCESS;
}

#if 0
/* load dynamic module */
static int loadModule(char symbol[], char path[])
{
	int r;
	const char *errMsg;
	module *tempModule;
	
	handle[dynamicModuleNum] = dlopen(path , RTLD_NOW);
	if (!(handle[dynamicModuleNum])) {
		error("%s",dlerror());
		return FAIL;
	} else {
		info("open [%s] module success",path);
	}

	tempModule = dlsym(handle[dynamicModuleNum], symbol);
	if ((errMsg = dlerror()) != NULL) {
		error(errMsg);
		return FAIL;
	} else {
		info("find [%s] symbol success",symbol);
	}

	// add module to g_module_array
	r = add_module(tempModule);
	if (r == FAIL) {
		return FAIL;
	} 
	
	dynamicModuleNum++;
	return SUCCESS;
}
#endif

/*
 * line parse & support parse section 
 */

/* check if line is begin with white space */
static int isWhiteSpace(char ch)
{
     if (ch == ' '  ||
			 ch == '\t' || 
			 ch == '\r' ||
			 ch == '\t' || 
			 ch == '\n' ||
			 ch == '\0')
	     return TRUE;

     return FAIL;
}

/* check if line is comment */
static int isComment(char ch)
{
	if (ch == '#')
		return TRUE;
	return FAIL;
}       

/* check if line is tag */
static int isTag(char ch)
{
	if (ch == '<')
		return TRUE;
	return FAIL;
}           

/* skip white space */
static void skipWhiteSpace(const char line[], int *i)
{
	int num = *i;
                
	while (isWhiteSpace(line[num]) == TRUE) {
		num++;
	};
        
	*i = num;
	return;
}

/* getConfig get only one argument 
 * So lineParse calls this function for each argument (delimited by white space)
 */
static void getConfig(const char line[], int *i ,char argument[])
{
    int  num = *i, pos = 0;

	if (line[num] == '\\') {
		if(line[num+1] == '"') {
			argument[pos++] = '"';
			num +=2;
		}
	}
	
	if (line[num] != '"') {
    // for general case 

        for ( ; ; ) {
			if (isWhiteSpace(line[num]) == TRUE) {
				argument[pos] = 0x00;
				*i = num;
				return;
			}

			argument[pos++] = line[num++];

			if (STRING_SIZE <= pos) {
				error("config length over [%s]",line);
				*i = num;
				return;
			}
        }

    } else if(line[num] == '"') {
    // case for use double quote " "    
        num++;

        while (line[num] != '"') {
			argument[pos++] = line[num++];

			if (STRING_SIZE <= pos) {
				error("config length over [%s]",line );
				*i = num;
				return;
			}

			if (line[num] == '\0') {
				error("not end dougle quote [%s]",line);
				break;
			}
		}

        argument[pos] = '\0';
        *i = num + 1;

        return;
    } else {
		argument[pos] = 0x00;
		*i = num;
		return;
	}
}

/* input line: a line in configuration file
 * output argument[0] is configuration name, argument[1:] is configuration parameter
 * returns number of parsed argument
 */ 
static int lineParse(const char line[], char (*argument)[STRING_SIZE])
{
    int numConfigArg = 0, i = 0;

    for (;;) {
		skipWhiteSpace(line , &i);
		if (i > strlen(line))
			break;

		if (line[i] == '#')
			break;

		getConfig(line, &i, &(argument[numConfigArg][0]));
		numConfigArg++;

		if (numConfigArg > MAX_CONFIG_ARG_NUM) {
			error("config argument number over");
			break;
		}
    }/* end for(;;) */
    return numConfigArg;
}

/* tag state
 * tag supporting section
 */

/* decide tag start or end check syntex error of tag 
 * not support nested tag
 */
static int makeTagState(char line[])
{
	if (line[1] != '/') {
		if (startTag(line) == FAIL) {
			error("start tag error [%s]",line);
			return FAIL;
		}
	} else {
		if (endTag(line) == FAIL) {
			error("end tag error [%s]",line);
			return FAIL;
		}
	}

    return TRUE;
}

/* start tag section ex) finds <lexicon.c>, <qpp.c>, etc */
static int startTag(char line[])
{
	int i = 0, nRet;

	if (strncmp(configTag , DEFAULT_TAG , sizeof(DEFAULT_TAG)) != 0) {
		error("configTag:%s, DEFAULT_TAG[%s] mismatch.",configTag,DEFAULT_TAG);
        return FAIL;
	}

    nRet = findCharacter(line , '>');
    if (nRet == FAIL) {
		error("tag does not end with '>'");
        return FAIL;
	}

    for (i = 0; i < nRet-1; i++) {
        configTag[i] = line[i+1];
        if (i == MAX_CONFIG_LINE) {
            error("tag length is longer than MAX_CONFIG_LINE(%d).",
					MAX_CONFIG_LINE);
            return FAIL;
        }
    }
    configTag[i] = '\0';

	if (check_config_syntax) {
		info(" Configuration for %s begins", configTag);
	}

    return SUCCESS;
}

/* end tag e.g.) finds </lexicon>,</qpp.c>, etc */
static int endTag(char line[])
{
	char buf[STRING_SIZE]="";
	char *right_paren=NULL;

	strncpy(buf,line,STRING_SIZE);
	buf[STRING_SIZE-1]='\0';

	right_paren=strchr(buf,'>');
	*right_paren='\0';

    if (strncmp(configTag , buf + 2/*'</'*/ , STRING_SIZE) != 0) {
		error("current tag:[%s], but it does not match with [%s]",
						configTag,	buf+2);
        return FAIL;
	}

	if (check_config_syntax) {
		info(" Configuration for %s end\n",buf+2);
	}

    copyString(configTag , DEFAULT_TAG);

    return SUCCESS;
}

/* find first index of given character in line */
static int findCharacter(char line[] , char ch)
{
    int i;

    for (i=0 ; line[i]!='\0' ; i++) {
        if(line[i] == ch )
            return i;
    }
    return FAIL;
}

/* section for memory operation */


/* wrapper for strncpy, with some error handling */
static void copyString(char *dest, const char *src)
{
#if 0
    int nLen=0;

    nLen = strlen(src);

    if (STRING_SIZE <= nLen) {
        error("src line over %d",strlen(src));
        nLen = STRING_SIZE;
    }

    strncpy(dest , src , nLen);

    dest[nLen-1] = '\0';
    return SUCCESS;
#endif
	/*
	 * FIXME: why check here?
	int len=0;
	len = strlen(src);
	if (len > STRING_SIZE) {
		error("config line is longer than %d",STRING_SIZE);
	}
	*/

	strncpy(dest,src,STRING_SIZE);
	dest[STRING_SIZE-1]='\0';
	return;
}

/* wrapper for strncmp */
static int isEqualString(const char *a, const char *b)
{
	if (strncmp(a,b, STRING_SIZE) != 0)
		return FALSE;

	return TRUE;
}
