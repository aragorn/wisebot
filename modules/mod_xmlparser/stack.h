/* $Id$ */
#ifndef _STACK_H_
#define _STACK_H_ 1

typedef struct _xmlparser_stack_t xmlparser_stack_t;

#if defined (__cplusplus)
extern "C" {
#endif

xmlparser_stack_t *st_create();
xmlparser_stack_t *st_create2(void *p, int size);
void st_destroy(xmlparser_stack_t *st);
int st_pop(xmlparser_stack_t *st, void **ptr, int *len);
int st_pop2(xmlparser_stack_t *st, void **ptr, int *len);
int st_push(xmlparser_stack_t *st, void *data, int len);
int st_append(xmlparser_stack_t *st, void *data, int len);

#if defined (__cplusplus)
}
#endif

#endif
