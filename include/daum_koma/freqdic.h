#ifndef _FREQDIC_H
#define _FREQDIC_H 1

int	fdict_init(dic_t *dic);
int	open_fdict(dic_t *dic);
void close_fdict(dic_t *dic);
int match_fdict(dic_t *dic,const char *zooword,void *outbuf,int bufsize);

#endif
