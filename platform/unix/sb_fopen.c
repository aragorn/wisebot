/*
 * $Id$
 */
#include "sb_fopen.h"

SB_DECLARE(FILE *) sb_fopen(const char *path, const char *mode)
{
/* BEGIN fopen(3) wrapper */
	char abs_path[MAX_PATH_LEN];
	struct stat buf;

	if (path[0] == '/') {
		snprintf(abs_path,MAX_PATH_LEN,"%s", path);
		warn("\"%s\" is an absolute path. use relative path.", path);
	} else
		snprintf(abs_path,MAX_PATH_LEN,"%s/%s", gSoftBotRoot, path);

	if ( stat(abs_path, &buf) == 0 ) {
		if ( S_ISLNK(buf.st_mode) )
			info("\"%s\" is a symbolic link", abs_path);
		else if ( ! S_ISREG(buf.st_mode) )
			info("\"%s\" is not a regular file", abs_path);
	}
/* END fopen(3) wrapper */
	
	return fopen(abs_path, mode);
}

SB_DECLARE(FILE *) sb_freopen(const char *path, const char *mode, FILE *stream)
{
	char abs_path[MAX_PATH_LEN];
	struct stat buf;

	if (path[0] == '/') {
		snprintf(abs_path,MAX_PATH_LEN,"%s", path);
		warn("\"%s\" is an absolute path. use relative path.", path);
	} else
		snprintf(abs_path,MAX_PATH_LEN,"%s/%s", gSoftBotRoot, path);

	if ( stat(abs_path, &buf) == 0 ) {
		if ( S_ISLNK(buf.st_mode) )
			info("\"%s\" is a symbolic link", abs_path);
		else if ( ! S_ISREG(buf.st_mode) )
			info("\"%s\" is not a regular file", abs_path);
	}
/* END fopen(3)/freopen(3) wrapper */
	
	return freopen(abs_path, mode, stream);
}
