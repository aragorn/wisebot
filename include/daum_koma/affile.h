/*
 * FILE : affile.h
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define READ_LINE_MAX	1024

#define DENIED_STATE	'A'
#define ACCEPTED_STATE	'B'

#define	NULL_CHAR		'\0'
#define TRUE			1
#define FALSE			0


typedef struct _node
{
	char curState;
	char curLevel;
	char curChar;

	struct _node* downNode;
	struct _node* nextNode;
} XNode;

XNode* loadSuffixFile (const char* file, char level);
XNode* loadPrefixFile (const char* file, char level);
void deleteXNode (XNode* node);

void displayXNodeList (XNode* node);	// 테스트용 메소트

