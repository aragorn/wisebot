#ifndef _CODELIB_H
#define _CODELIB_H 1

int HANL_hanja2ks_c2(unsigned char h1, unsigned char h2, unsigned char *k1, unsigned char *k2);

int HANL_is_hanja(unsigned char c1, unsigned char c2);

int HANL_hanja2ks_str(unsigned char *hanja, unsigned char *hangul);

int HANL_is_hangul(unsigned char c1,unsigned char c2);

int HANL_make2c(unsigned char cho, unsigned char jung1, unsigned char jung2, unsigned char jong1, unsigned char jong2);

int HANL_whattype(unsigned char ch);

int HANL_kimmo2ks(unsigned char *src,unsigned char *des);

int HANL_is_2vowel(unsigned char c);

int HANL_is_jungsung(unsigned char c);

int HANL_is_chosung(unsigned char c);

int HANL_is_jongsung(unsigned char c);

int HANL_Replace_2vowel_to_1vowel(unsigned char * zooword);

int HANL_ks2kimmo(unsigned char *src,unsigned char *des);

int HANL_ks2kimmo2(unsigned char *src, unsigned char *des, int *whereis);

int HANL_ks(unsigned char *src, unsigned char *des, int type);

int HANL_syllable(int src, int type);

int HANL_binsrch(int code[][2], int n, int key, int type);

void HANL_reverse(char *s1,char *s2);

int HANL_preprocess_doi(unsigned char * zooword);

int HANL_make_morpheme(int start,int end,unsigned char *word,unsigned char *morpheme);

#endif
