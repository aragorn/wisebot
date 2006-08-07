/**************************************************************************
 *
 * @ ���� ���� �˻��� ���� ���
 * 
 * 1. ��Ʋ�� ��� �̿��Ͽ� ������ �ۼ�
 * 2. ���� DHA�� trie ������ �����Ͽ� ����
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>
#include <db1/db.h>
#include <fcntl.h>

#define DB_RECORD_MAX	1024

int createDBFile (const char* dbFile, const char* inFile);
DB* openDBFile (const char* dbFile);
void closeDBFile (DB* db);
char* getSearchedItem (DB* db, char* item);
