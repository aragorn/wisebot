/**************************************************************************
 *
 * @ 사전 파일 검색을 위한 모듈
 * 
 * 1. 버틀리 디비를 이용하여 일차적 작성
 * 2. 추후 DHA의 trie 구조로 수정하여 붙임
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
