/* $Id$ */
#include "softbot.h"
#include "mod_api/docapi.h"

#define TEST_DATA			4

static char *testfields[] = { "field1", "field2", "field3", "field4" };
static char *testdata[] = {
	"testdata1",
	"To use your new extension, you will have to execute the following steps:",
	"nominam@white:~/work/php-4.2.3/ext$ ./ext_skel --extname=sbsearch",
	"1.  $ cd ..\n"
	"2.  $ vi ext/sbsearch/config.m4\n"
	"3.  $ ./buildconf\n"
	"4.  $ ./configure --[with|enable]-sbsearch\n"
	"5.  $ make\n"
	"6.  $ ./php -f ext/sbsearch/sbsearch.php\n"
	"7.  $ vi ext/sbsearch/sbsearch.c\n"
	"8.  $ make\n"
    "\n"
	"Repeat steps 3-6 until you are satisfied with ext/sbsearch/config.m4 and\n"
	"step 6 confirms that your module is compiled into PHP. Then, start writing\n"
	"code and repeat the last two steps as often as necessary.\n"
};


static int module_main(slot_t *slot)
{
	document_t *doc;
	void *ptr;
	int len, i;
	doc = sb_run_docapi_initdoc();
	if (doc == NULL) {
		error("cannot init doc");
		return 1;
	}
	for (i=0; i<TEST_DATA; i++) {
		if (sb_run_docapi_add_field(doc, testfields[i], testdata[i], 
					strlen(testdata[i])) == -1) {
			error("cannot add field");
			return 1;
		}
		if (sb_run_docapi_get_field(doc, "field1", &ptr, &len) == -1) {
			error("cannot get field");
			return 1;
		}
		debug("field[%s]: %s", testfields[i], (char *)ptr);
	}
	sb_run_docapi_freedoc(doc);
	return 0;
}

module docapi_test_module = {
	TEST_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	module_main,			/* child_main */
	NULL,					/* scoreboard */
	NULL					/* register hook api */
};
