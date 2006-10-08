/* $Id$ */
#ifndef __HTTP_UTIL_H__
#define __HTTP_UTIL_H__

#include "mod_httpd.h"
#include "pcreposix.h"
#include "http_config.h"


/**
 * Examine a field value (such as a media-/content-type) string and return
 * it sans any parameters; e.g., strip off any ';charset=foo' and the like.
 * @param p Pool to allocate memory from
 * @param intype The field to examine
 * @return A copy of the field minus any parameters
 */
AP_DECLARE(char *) ap_field_noparam(apr_pool_t *p, const char *intype);

/**
 * Convert a time from an integer into a string in a specified format
 * @param p The pool to allocate memory from
 * @param t The time to convert
 * @param fmt The format to use for the conversion
 * @param gmt Convert the time for GMT?
 * @return The string that represents the specified time
 */
AP_DECLARE(char *) ap_ht_time(apr_pool_t *p, apr_time_t t, const char *fmt, int gmt);

/* String handling. The *_nc variants allow you to use non-const char **s as
   arguments (unfortunately C won't automatically convert a char ** to a const
   char **) */

/**
 * Get the characters until the first occurance of a specified character
 * @param p The pool to allocate memory from
 * @param line The string to get the characters from
 * @param stop The character to stop at
 * @return A copy of the characters up to the first stop character
 */
AP_DECLARE(char *) ap_getword(apr_pool_t *p, const char **line, char stop);
/**
 * Get the characters until the first occurance of a specified character
 * @param p The pool to allocate memory from
 * @param line The string to get the characters from
 * @param stop The character to stop at
 * @return A copy of the characters up to the first stop character
 * @note This is the same as ap_getword(), except it doesn't use const char **.
 */
AP_DECLARE(char *) ap_getword_nc(apr_pool_t *p, char **line, char stop);

/**
 * Get the first word from a given string.  A word is defined as all characters
 * up to the first whitespace.
 * @param p The pool to allocate memory from
 * @param line The string to traverse
 * @return The first word in the line
 */
AP_DECLARE(char *) ap_getword_white(apr_pool_t *p, const char **line);
/**
 * Get the first word from a given string.  A word is defined as all characters
 * up to the first whitespace.
 * @param p The pool to allocate memory from
 * @param line The string to traverse
 * @return The first word in the line
 * @note The same as ap_getword_white(), except it doesn't use const char **.
 */
AP_DECLARE(char *) ap_getword_white_nc(apr_pool_t *p, char **line);

/**
 * Get all characters from the first occurance of @a stop to the first '\0'
 * @param p The pool to allocate memory from
 * @param line The line to traverse
 * @param stop The character to start at
 * @return A copy of all caracters after the first occurance of the specified
 *         character
 */
AP_DECLARE(char *) ap_getword_nulls(apr_pool_t *p, const char **line,
				    char stop);
/**
 * Get all characters from the first occurance of @a stop to the first '\0'
 * @param p The pool to allocate memory from
 * @param line The line to traverse
 * @param stop The character to start at
 * @return A copy of all caracters after the first occurance of the specified
 *         character
 * @note The same as ap_getword_nulls(), except it doesn't use const char **.
 */
AP_DECLARE(char *) ap_getword_nulls_nc(apr_pool_t *p, char **line, char stop);

/**
 * Get the second word in the string paying attention to quoting
 * @param p The pool to allocate from
 * @param line The line to traverse
 * @return A copy of the string
 */
AP_DECLARE(char *) ap_getword_conf(apr_pool_t *p, const char **line);
/**
 * Get the second word in the string paying attention to quoting
 * @param p The pool to allocate from
 * @param line The line to traverse
 * @return A copy of the string
 * @note The same as ap_getword_conf(), except it doesn't use const char **.
 */
AP_DECLARE(char *) ap_getword_conf_nc(apr_pool_t *p, char **line);

/**
 * Check a string for any ${ENV} environment variable construct and replace 
 * each them by the value of that environment variable, if it exists. If the 
 * environment value does not exist, leave the ${ENV} construct alone; it 
 * means something else.
 * @param p The pool to allocate from
 * @param word The string to check
 * @return The string with the replaced environment variables
 */
AP_DECLARE(const char *) ap_resolve_env(apr_pool_t *p, const char * word); 

/*****************************************************************************/
/**
 * Open a ap_configfile_t as apr_file_t
 * @param ret_cfg open ap_configfile_t struct pointer
 * @param p The pool to allocate the structure from
 * @param name the name of the file to open
 */
AP_DECLARE(apr_status_t) ap_pcfg_openfile(ap_configfile_t **ret_cfg,
                                          apr_pool_t *p, const char *name);

/**
 * Allocate a ap_configfile_t handle with user defined functions and params 
 * @param p The pool to allocate from
 * @param descr The name of the file
 * @param param The argument passed to getch/getstr/close
 * @param getc_func The getch function
 * @param gets_func The getstr function
 * @param close_func The close function
 */
#if 0
AP_DECLARE(ap_configfile_t *) ap_pcfg_open_custom(apr_pool_t *p, 
    const char *descr,
    void *param,
    int(*getc_func)(void*),
    void *(*gets_func) (void *buf, size_t bufsiz, void *param),
    int(*close_func)(void *param));
#endif
/**
 * Read one line from open ap_configfile_t, strip LF, increase line number
 * @param buf place to store the line read
 * @param bufsize size of the buffer
 * @param cfp File to read from
 * @return 1 on success, 0 on failure
 */
AP_DECLARE(int) ap_cfg_getline(char *buf, size_t bufsize, ap_configfile_t *cfp);

/**
 * Read one char from open configfile_t, increase line number upon LF 
 * @param cfp The file to read from
 * @return the character read
 */
AP_DECLARE(int) ap_cfg_getc(ap_configfile_t *cfp);

/**                                       
 * Detach from open ap_configfile_t, calling the close handler
 * @param cfp The file to close
 * @return 1 on sucess, 0 on failure
 */
AP_DECLARE(int) ap_cfg_closefile(ap_configfile_t *cfp);

/*****************************************************************************/

/**
 * Size an HTTP header field list item, as separated by a comma.
 * @param field The field to size
 * @param len The length of the field
 * @return The return value is a pointer to the beginning of the non-empty 
 * list item within the original string (or NULL if there is none) and the 
 * address of field is shifted to the next non-comma, non-whitespace 
 * character.  len is the length of the item excluding any beginning whitespace.
 */
AP_DECLARE(const char *) ap_size_list_item(const char **field, int *len);

/**
 * Retrieve an HTTP header field list item, as separated by a comma,
 * while stripping insignificant whitespace and lowercasing anything not in
 * a quoted string or comment.  
 * @param p The pool to allocate from
 * @param field The field to retrieve
 * @return The return value is a new string containing the converted list 
 *         item (or NULL if none) and the address pointed to by field is 
 *         shifted to the next non-comma, non-whitespace.
 */
AP_DECLARE(char *) ap_get_list_item(apr_pool_t *p, const char **field);

/**
 * Find an item in canonical form (lowercase, no extra spaces) within
 * an HTTP field value list.  
 * @param p The pool to allocate from
 * @param line The field value list to search
 * @param tok The token to search for
 * @return 1 if found, 0 if not found.
 */
AP_DECLARE(int) ap_find_list_item(apr_pool_t *p, const char *line, const char *tok);

/**
 * Retrieve a token, spacing over it and returning a pointer to
 * the first non-white byte afterwards.  Note that these tokens
 * are delimited by semis and commas and can also be delimited
 * by whitespace at the caller's option.
 * @param p The pool to allocate from
 * @param accept_line The line to retrieve the token from
 * @param accept_white Is it delimited by whitespace
 * @return the first non-white byte after the token
 */
AP_DECLARE(char *) ap_get_token(apr_pool_t *p, const char **accept_line, int accept_white);

/**
 * Find http tokens, see the definition of token from RFC2068 
 * @param p The pool to allocate from
 * @param line The line to find the token
 * @param tok The token to find
 * @return 1 if the token is found, 0 otherwise
 */
AP_DECLARE(int) ap_find_token(apr_pool_t *p, const char *line, const char *tok);

/**
 * find http tokens from the end of the line
 * @param p The pool to allocate from
 * @param line The line to find the token
 * @param tok The token to find
 * @return 1 if the token is found, 0 otherwise
 */
AP_DECLARE(int) ap_find_last_token(apr_pool_t *p, const char *line, const char *tok);

/**
 * Check for an Absolute URI syntax
 * @param u The string to check
 * @return 1 if URI, 0 otherwise
 */
AP_DECLARE(int) ap_is_url(const char *u);

/**
 * Unescape a URL
 * @param url The url to unescapte
 * @return 0 on success, non-zero otherwise
 */
AP_DECLARE(int) ap_unescape_url(char *url);
/**
 * Convert all double slashes to single slashes
 * @param name The string to convert
 */
AP_DECLARE(void) ap_no2slash(char *name);

/**
 * Remove all ./ and xx/../ substrings from a file name. Also remove
 * any leading ../ or /../ substrings.
 * @param name the file name to parse
 */
AP_DECLARE(void) ap_getparents(char *name);

/**
 * Escape a path segment, as defined in RFC 1808
 * @param p The pool to allocate from
 * @param s The path to convert
 * @return The converted URL
 */
AP_DECLARE(char *) ap_escape_path_segment(apr_pool_t *p, const char *s);
/**
 * convert an OS path to a URL in an OS dependant way.
 * @param p The pool to allocate from
 * @param path The path to convert
 * @param partial if set, assume that the path will be appended to something
 *        with a '/' in it (and thus does not prefix "./")
 * @return The converted URL
 */
AP_DECLARE(char *) ap_os_escape_path(apr_pool_t *p, const char *path, int partial);
/** @see ap_os_escape_path */
#define ap_escape_uri(ppool,path) ap_os_escape_path(ppool,path,1)

// apr_pool_t를 쓰지 않는 함수
AP_DECLARE(char *) escape_path(const char *path, char* copy);

/**
 * Escape an html string
 * @param p The pool to allocate from
 * @param s The html to escape
 * @return The escaped string
 */
AP_DECLARE(char *) ap_escape_html(apr_pool_t *p, const char *s);

/**
 * Construct a full hostname
 * @param p The pool to allocate from
 * @param hostname The hostname of the server
 * @param port The port the server is running on
 * @param r The current request
 * @return The server's hostname
 */
AP_DECLARE(char *) ap_construct_server(apr_pool_t *p, const char *hostname,
				    apr_port_t port, const request_rec *r);
/**
 * Escape a shell command
 * @param p The pool to allocate from
 * @param s The command to escape
 * @return The escaped shell command
 */
AP_DECLARE(char *) ap_escape_shell_cmd(apr_pool_t *p, const char *s);

/**
 * Count the number of directories in a path
 * @param path The path to count
 * @return The number of directories
 */
AP_DECLARE(int) ap_count_dirs(const char *path);

/**
 * Copy at most @a n leading directories of @a s into @a d. @a d
 * should be at least as large as @a s plus 1 extra byte
 *
 * @param d The location to copy to
 * @param s The location to copy from
 * @param n The number of directories to copy
 * @return value is the ever useful pointer to the trailing \0 of d
 * @note on platforms with drive letters, n = 0 returns the "/" root, 
 * whereas n = 1 returns the "d:/" root.  On all other platforms, n = 0
 * returns the empty string.  */
AP_DECLARE(char *) ap_make_dirstr_prefix(char *d, const char *s, int n);

/**
 * Return the parent directory name (including trailing /) of the file
 * @a s
 * @param p The pool to allocate from
 * @param s The file to get the parent of
 * @return A copy of the file's parent directory
 */
AP_DECLARE(char *) ap_make_dirstr_parent(apr_pool_t *p, const char *s);

/**
 * Given a directory and filename, create a single path from them.  This
 * function is smart enough to ensure that there is a sinlge '/' between the
 * directory and file names
 * @param a The pool to allocate from
 * @param dir The directory name
 * @param f The filename
 * @return A copy of the full path
 * @tip Never consider using this function if you are dealing with filesystem
 * names that need to remain canonical, unless you are merging an apr_dir_read
 * path and returned filename.  Otherwise, the result is not canonical.
 */
AP_DECLARE(char *) ap_make_full_path(apr_pool_t *a, const char *dir, const char *f);

/**
 * Test if the given path has an an absolute path.
 * @param p The pool to allocate from
 * @param dir The directory name
 * @tip The converse is not necessarily true, some OS's (Win32/OS2/Netware) have
 * multiple forms of absolute paths.  This only reports if the path is absolute
 * in a canonical sense.
 */
AP_DECLARE(int) ap_os_is_path_absolute(apr_pool_t *p, const char *dir);

/**
 * Does the provided string contain wildcard characters?  This is useful
 * for determining if the string should be passed to strcmp_match or to strcmp.
 * The only wildcard characters recognized are '?' and '*'
 * @param str The string to check
 * @return 1 if the string has wildcards, 0 otherwise
 */
AP_DECLARE(int) ap_is_matchexp(const char *str);

/**
 * Determine if a string matches a patterm containing the wildcards '?' or '*'
 * @param str The string to check
 * @param exp The pattern to match against
 * @return 1 if the two strings match, 0 otherwise
 */
AP_DECLARE(int) ap_strcmp_match(const char *str, const char *exp);
/**
 * Determine if a string matches a patterm containing the wildcards '?' or '*',
 * ignoring case
 * @param str The string to check
 * @param exp The pattern to match against
 * @return 1 if the two strings match, 0 otherwise
 */
AP_DECLARE(int) ap_strcasecmp_match(const char *str, const char *exp);

/**
 * Find the first occurrence of the substring s2 in s1, regardless of case
 * @param s1 The string to search
 * @param s2 The substring to search for
 * @return A pointer to the beginning of the substring
 */
AP_DECLARE(char *) ap_strcasestr(const char *s1, const char *s2);

/**
 * Return a pointer to the location inside of bigstring immediately after prefix
 * @param bigstring The input string
 * @param prefix The prefix to strip away
 * @return A pointer relative to bigstring after prefix
 */
AP_DECLARE(const char *) ap_stripprefix(const char *bigstring,
                                        const char *prefix);

/**
 * Decode a base64 encoded string into memory allocated from a pool
 * @param p The pool to allocate from
 * @param bufcoded The encoded string
 * @return The decoded string
 */
AP_DECLARE(char *) ap_pbase64decode(apr_pool_t *p, const char *bufcoded);

/**
 * Encode a string into memory allocated from a pool in base 64 format
 * @param p The pool to allocate from
 * @param strin The plaintext string
 * @return The encoded string
 */
AP_DECLARE(char *) ap_pbase64encode(apr_pool_t *p, char *string); 


/*** pcre stuff **************************************************************/
/**
 * Compile a regular expression to be used later
 * @param p The pool to allocate from
 * @param pattern the regular expression to compile
 * @param cflags The bitwise or of one or more of the following:
 *   @li #REG_EXTENDED - Use POSIX extended Regular Expressions
 *   @li #REG_ICASE    - Ignore case
 *   @li #REG_NOSUB    - Support for substring addressing of matches
 *       not required
 *   @li #REG_NEWLINE  - Match-any-character operators don't match new-line
 * @return The compiled regular expression
 */
AP_DECLARE(regex_t *) ap_pregcomp(apr_pool_t *p, const char *pattern,
				   int cflags);

/**
 * Free the memory associated with a compiled regular expression
 * @param p The pool the regex was allocated from
 * @param reg The regular expression to free
 */
AP_DECLARE(void) ap_pregfree(apr_pool_t *p, regex_t *reg);

/**
 * Match a null-terminated string against a pre-compiled regex.
 * @param preg The pre-compiled regex
 * @param string The string to match
 * @param nmatch Provide information regarding the location of any matches
 * @param pmatch Provide information regarding the location of any matches
 * @param eflags Bitwise or of any of:
 *   @li #REG_NOTBOL - match-beginning-of-line operator always
 *     fails to match
 *   @li #REG_NOTEOL - match-end-of-line operator always fails to match
 * @return 0 for successful match, #REG_NOMATCH otherwise
 */ 
AP_DECLARE(int)    ap_regexec(regex_t *preg, const char *string,
                              size_t nmatch, regmatch_t pmatch[], int eflags);

/**
 * Return the error code returned by regcomp or regexec into error messages
 * @param errcode the error code returned by regexec or regcomp
 * @param preg The precompiled regex
 * @param errbuf A buffer to store the error in
 * @param errbuf_size The size of the buffer
 */
AP_DECLARE(size_t) ap_regerror(int errcode, const regex_t *preg, 
                               char *errbuf, size_t errbuf_size);

/**
 * After performing a successful regex match, you may use this function to 
 * perform a series of string substitutions based on subexpressions that were
 * matched during the call to ap_regexec
 * @param p The pool to allocate from
 * @param input An arbitrary string containing $1 through $9.  These are 
 *              replaced with the corresponding matched sub-expressions
 * @param source The string that was originally matched to the regex
 * @param nmatch the nmatch returned from ap_pregex
 * @param pmatch the pmatch array returned from ap_pregex
 */
AP_DECLARE(char *) ap_pregsub(apr_pool_t *p, const char *input, const char *source,
                              size_t nmatch, regmatch_t pmatch[]);



/**
 * We want to downcase the type/subtype for comparison purposes
 * but nothing else because ;parameter=foo values are case sensitive.
 * @param s The content-type to convert to lowercase
 */
AP_DECLARE(void) ap_content_type_tolower(char *s);

/**
 * convert a string to all lowercase
 * @param s The string to convert to lowercase 
 */
AP_DECLARE(void) ap_str_tolower(char *s);

/**
 * Search a string from left to right for the first occurrence of a 
 * specific character
 * @param str The string to search
 * @param c The character to search for
 * @return The index of the first occurrence of c in str
 */
AP_DECLARE(int) ap_ind(const char *str, char c);	/* Sigh... */

/**
 * Search a string from right to left for the first occurrence of a 
 * specific character
 * @param str The string to search
 * @param c The character to search for
 * @return The index of the first occurrence of c in str
 */
AP_DECLARE(int) ap_rind(const char *str, char c);

/**
 * Given a string, replace any bare " with \" .
 * @param p The pool to allocate memory from
 * @param instring The string to search for "
 * @return A copy of the string with escaped quotes 
 */
AP_DECLARE(char *) ap_escape_quotes(apr_pool_t *p, const char *instring);

/* Misc system hackery */
/**
 * Given the name of an object in the file system determine if it is a directory
 * @param p The pool to allocate from 
 * @param name The name of the object to check
 * @return 1 if it is a directory, 0 otherwise
 */
AP_DECLARE(int) ap_is_rdirectory(apr_pool_t *p, const char *name);

/**
 * Given the name of an object in the file system determine if it is a directory - this version is symlink aware
 * @param p The pool to allocate from 
 * @param name The name of the object to check
 * @return 1 if it is a directory, 0 otherwise
 */
AP_DECLARE(int) ap_is_directory(apr_pool_t *p, const char *name);

#ifdef _OSD_POSIX
extern const char *os_set_account(apr_pool_t *p, const char *account);
extern int os_init_job_environment(server_rec *s, const char *user_name, int one_process);
#endif /* _OSD_POSIX */

/**
 * Determine the local host name for the current machine
 * @param p The pool to allocate from
 * @return A copy of the local host name
 */
char *ap_get_local_host(apr_pool_t *p);

#if 0
/**
 * Log an assertion to the error log
 * @param szExp The assertion that failed
 * @param szFile The file the assertion is in
 * @param nLine The line the assertion is defined on
 */
AP_DECLARE(void) ap_log_assert(const char *szExp, const char *szFile, int nLine)
			    __attribute__((noreturn));

/** @internal */
#define ap_assert(exp) ((exp) ? (void)0 : ap_log_assert(#exp,__FILE__,__LINE__))

/**
 * Redefine assert() to something more useful for an Apache...
 *
 * Use ap_assert() if the condition should always be checked.
 * Use AP_DEBUG_ASSERT() if the condition should only be checked when AP_DEBUG
 * is defined.
 */

#ifdef AP_DEBUG
#define AP_DEBUG_ASSERT(exp) ap_assert(exp)
#else
#define AP_DEBUG_ASSERT(exp) ((void)0)
#endif

#endif //0

/**
 * @defgroup stopsignal flags which indicate places where the sever should stop for debugging.
 * @{
 * A set of flags which indicate places where the server should raise(SIGSTOP).
 * This is useful for debugging, because you can then attach to that process
 * with gdb and continue.  This is important in cases where one_process
 * debugging isn't possible.
 */
/** stop on a Detach */
#define SIGSTOP_DETACH			1
/** stop making a child process */
#define SIGSTOP_MAKE_CHILD		2
/** stop spawning a child process */
#define SIGSTOP_SPAWN_CHILD		4
/** stop spawning a child process with a piped log */
#define SIGSTOP_PIPED_LOG_SPAWN		8
/** stop spawning a CGI child process */
#define SIGSTOP_CGI_CHILD		16

/** Macro to get GDB started */
#ifdef DEBUG_SIGSTOP
extern int raise_sigstop_flags;
#define RAISE_SIGSTOP(x)	do { \
	if (raise_sigstop_flags & SIGSTOP_##x) raise(SIGSTOP);\
    } while (0)
#else
#define RAISE_SIGSTOP(x)
#endif
/** @} */
/**
 * Get HTML describing the address and (optionally) admin of the server.
 * @param prefix Text which is prepended to the return value
 * @param r The request_rec
 * @return HTML describing the server, allocated in @a r's pool.
 */
//AP_DECLARE(const char *) ap_psignature(const char *prefix, request_rec *r);

/** strtoul does not exist on sunos4. */
#ifdef strtoul
#undef strtoul
#endif
#define strtoul strtoul_is_not_a_portable_function_use_strtol_instead

  /* The C library has functions that allow const to be silently dropped ...
     these macros detect the drop in maintainer mode, but use the native
     methods for normal builds

     Note that on some platforms (e.g., AIX with gcc, Solaris with gcc), string.h needs 
     to be included before the macros are defined or compilation will fail.
  */
#include <string.h>

#ifdef AP_DEBUG

#undef strchr
# define strchr(s, c)	ap_strchr(s,c)
#undef strrchr
# define strrchr(s, c)  ap_strrchr(s,c)
#undef strstr
# define strstr(s, c)  ap_strstr(s,c)

AP_DECLARE(char *) ap_strchr(char *s, int c);
AP_DECLARE(const char *) ap_strchr_c(const char *s, int c);
AP_DECLARE(char *) ap_strrchr(char *s, int c);
AP_DECLARE(const char *) ap_strrchr_c(const char *s, int c);
AP_DECLARE(char *) ap_strstr(char *s, const char *c);
AP_DECLARE(const char *) ap_strstr_c(const char *s, const char *c);

#else

/** use this instead of strchr */
# define ap_strchr(s, c)	strchr(s, c)
/** use this instead of strchr */
# define ap_strchr_c(s, c)	strchr(s, c)
/** use this instead of strrchr */
# define ap_strrchr(s, c)	strrchr(s, c)
/** use this instead of strrchr */
# define ap_strrchr_c(s, c)	strrchr(s, c)
/** use this instead of strrstr*/
# define ap_strstr(s, c)	strstr(s, c)
/** use this instead of strrstr*/
# define ap_strstr_c(s, c)	strstr(s, c)

#endif

#define AP_NORESTART		APR_OS_START_USEERR + 1

/* from util_script.c */
AP_DECLARE(int) ap_find_path_info(const char *uri, const char *path_info);

#ifdef __cplusplus
}
#endif

#endif  /* !__UTIL_H__ */
