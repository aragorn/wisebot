/* $Id$ */
#include "mod_koma.h"

int main()
{
	koma_handler_t	*handler;
	int i, num=-1;
	int	server_id =0;
	char input[]="ùÓÙþ°ú ÇÑ±Û";
	index_word_t	index_word[100];
	char text[1024];

	if (LoadKomaEngine (MainFSTFilename, MainDataFilename, ConnectionTableFilename, 
					TagFilename, TagOutFilename) == false) {
		fprintf(stderr, "ERROR :: cannot load KOMA engine\n");
		return 1;
	}

	if (LoadHanTagEngine(ProbEntryFilename, ProbDataFilename) == false) {
		fprintf(stderr, "ERROR :: cannot load HanTag engine\n");
		return 1;
	}

	handler = new_koma_analyzer(server_id);

	while (1) {
		printf("Enter text: ");
		fgets(text, 1024, stdin);
		koma_set_text(handler, (char *)text);

		if((num = koma_analyzer(handler, index_word, 30))>0) {
			for(i=0;i<num;i++) {
				fprintf(stderr, "Ã³¸® °á°ú : ");
				fprintf(stderr, "word : %s " , index_word[i].word);
				fprintf(stderr, "len : %d " , index_word[i].len);
				fprintf(stderr, "pos : %d \n" , index_word[i].pos);
			}
		}
		break;
	}

	fprintf(stderr, "total count : %d \n", num);
	delete_koma_analyzer(handler);

	return 0;
}
