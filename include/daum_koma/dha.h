#ifndef _DHA_H
#define _DHA_H 1

#ifdef __cplusplus 
extern "C" { 
#endif

/*
 * initialize dha module
 *
 * @param path - the path to dictionaries
 * @return - pointer to object
 */
void *dha_initialize(const char *szDicPath,const char *szConfPath);

/*
 * finalize dha module
 *
 * @param this - the pointer of object
 */
void dha_finalize(void *dha);

/*
 * analyze hangul string & return the list of morpheme
 *
 * @param this - the pointer of analyzer object
 * @param option - option -- obsolete
 * @param str - input string
 * @param bufsize - length of _buf_
 * @param buf - buffer for output
 *
 * @return if success, 1, otherwise -1
 */
int dha_analyze(void *dha, const char *option, char *string, int bufsize, char *buf);

int HANL_ks2kimmo(unsigned char *src,unsigned char *des);
int	SetConfig(char	*str);
int	Check_BOOLEAN();
#ifdef __cplusplus 
}
#endif

#endif
