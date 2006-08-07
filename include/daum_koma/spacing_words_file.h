#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <db1/db.h>
#include <fcntl.h>
#include "BinarySearch.h"
#define	MAXWORDSIZE 8
#define SUCCESS 1
#define FAIL    0
#define WEIGHT 0.25
#include "cgi-llist.h"
#define MAX	102400
#define MIN 102400
#define KOREAN 0x80
#define THRESHOLD 220
#define MTHRESHOLD 150
#define LEFT  "/data3/daumsoft/SPACING_WORDS/BTREE/L.BDB"
#define RIGHT  "/data3/daumsoft/SPACING_WORDS/BTREE/R.BDB"
#define MIDDLE  "/data3/daumsoft/SPACING_WORDS/BTREE/M.BDB"
#define UNIDIC  "/data3/daumsoft/SPACING_WORDS/DIC/U.DIC"
#define SUNIDIC  "/data3/daumsoft/SPACING_WORDS/DIC/KOR.DIC"
//#define SPECIALDIC  "/data3/daumsoft/SPACING_WORDS/DIC/MAN.TRIE"
#define SPECIALDIC  "/data3/daumsoft/SPACING_WORDS/DIC/SPECIAL.TRIE"

int		CheckKorean(char * str);
int		probability(char * string, int*point, char * DIC[LEN]);
void 	Spacing_words_file(char * buffer, char * reslt, char * div,char * DIC[LEN]);
char 	*Print(char * word, char * result, int	* space, int size, char * div,int mod);
