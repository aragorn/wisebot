/* $Id$ */
#include "mod_indexer_test.h"

#include <stdio.h>
#include <glib.h>
#include <c-unit/test.h>
#include <string.h>
#include <assert.h>

int printDebug=0;
int nRet=0;
char path[256]=TEST_DB_PATH"/indexer/";
UInt32 i=0,j=0,k=0;
InvIdxHeader invIdxHeader;
InvertedIndex invertedIndex;
DocId startDid=0;
UInt32 numToProcessIndex=0;
UInt32 currentIdx=0;
WordId *wordIds=NULL;
DocId *docIds=NULL;
DocHit *docHits=NULL;

gint setUpIndexer(autounit_test_t *t) {
	nRet = 0;
	i=0,j=0,k=0;

	return TRUE;
}

gint tearDownIndexer(autounit_test_t *t) {
	return TRUE;
}

void init_once() {
	// FIXME lexicon manager와 합칠 때 다시 바뀌어야 한다!
/*	SP_setPath(TEST_CONFIG_PATH"/conf_for_indexer");*/
/*	LM_init(TEST_CONFIG_PATH"/conf_for_indexer");*/
/*	LM_load(SUFFIX_ON);*/
	_setDicFile(TEST_DB_PATH"/dic/ma.dic");
}

gint testSimple(autounit_test_t *t) {
	indexer_main(0,NULL);

	return TRUE;
}

typedef struct {
	gboolean forking;
    char *name;
    autounit_test_fp_t test_fp;
    gboolean isEnabled;
} test_link_t;

static test_link_t tests[] = {
	{FORK_NOT,"",testSimple,TRUE},
	{FORK_NOT,0,0,FALSE}
};

int
main(int argc, char** argv) {
    autounit_testcase_t *test_indexer;
    int test_no;
    gint result;
    autounit_test_t *tmp_test;
    
    test_indexer = 
        au_new_testcase(g_string_new("Indexer TestCase"),
                        setUpIndexer,tearDownIndexer);
    
    test_no = 0;
    while (tests[test_no].name != 0) {
        if (tests[test_no].isEnabled == TRUE) {
            tmp_test = au_new_test(g_string_new(tests[test_no].name),
                                tests[test_no].test_fp);
			au_set_fork_mode(tmp_test,tests[test_no].forking);
            au_add_test(test_indexer,tmp_test);
        }
        else {
            printf("  ?! '%s' test disabled\n",tests[test_no].name);
        }
        test_no++;
    }

	if (argc > 1) {
		if (strcasecmp(argv[1],"debug") == 0) {
			printDebug = 1;
		}
	}
    
	init_once();
    result = au_run_testcase(test_indexer);
	return result;
}
