/*
 * $Id$
 */
#include "sb_open.h"

SB_DECLARE(int) sb_open(const char *path, int flags, ...)
{
	char abs_path[MAX_PATH_LEN];
	struct stat buf;
	va_list ap;
	mode_t mode;

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

	va_start(ap, flags);
#ifdef HPUX
	mode = (mode_t) va_arg(ap, int);
#else
	mode = va_arg(ap, mode_t);
#endif
	va_end(ap);

	if ( O_CREAT&flags ) /* see end of open(2) DESCRIPTION */
		return open(abs_path, flags, mode);
	else
		return open(abs_path, flags);
}

SB_DECLARE(int) sb_unlink(const char *path)
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

	return unlink(abs_path);
}


SB_DECLARE(int) sb_rename(const char *oldpath, const char *newpath)
{
	char abs_oldpath[MAX_PATH_LEN];
	char abs_newpath[MAX_PATH_LEN];

	/* old path */
	if (oldpath[0] == '/') {
		snprintf(abs_oldpath,MAX_PATH_LEN,"%s", oldpath);
		warn("\"%s\" is an absolute path. use relative path.", oldpath);
	} else
		snprintf(abs_oldpath,MAX_PATH_LEN,"%s/%s", gSoftBotRoot, oldpath);

	/* new path */
	if (newpath[0] == '/') {
		snprintf(abs_newpath,MAX_PATH_LEN,"%s", newpath);
		warn("\"%s\" is an absolute path. use relative path.", newpath);
	} else
		snprintf(abs_newpath,MAX_PATH_LEN,"%s/%s", gSoftBotRoot, newpath);


	return rename(abs_oldpath, abs_newpath);
}

