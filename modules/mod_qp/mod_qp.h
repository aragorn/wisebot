/* $Id$ */
#ifndef MOD_QP_H
#define MOD_QP_H 1

///////////////////////////////////////////////////////////
// �������Ͽ��� �о�� �ʵ��� �Ӽ�
enum field_type {
    NONE,           // �ƹ�ó������ ����.
	RETURN,         // �ܼ����
    SUM,            // ������	
	SUM_OR_FIRST,   // �ܾ ������ ������
};

// �ʵ��, �ʵ� �Ӽ�
typedef struct {
    char name[SHORT_STRING_SIZE];
    int type;       // enum field_type
    int id;         // field id
} field_info_t;
///////////////////////////////////////////////////////////


/* stack related stuff */
typedef struct {
	uint32_t size;
	struct index_list_t *first;
	struct index_list_t *last;
} sb_stack_t;

void init_stack(sb_stack_t *stack);
void stack_push(sb_stack_t *stack, struct index_list_t *this);
struct index_list_t *stack_pop(sb_stack_t *stack);
/* stack related stuff end */

int light_search (void* word_db, struct request_t *req);
int getAutoComment(char *pszStr, int lPosition);

#endif
