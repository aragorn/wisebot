/* $Id$ */
#ifndef MOD_QP_H
#define MOD_QP_H 1

///////////////////////////////////////////////////////////
// 설정파일에서 읽어온 필드의 속성
enum field_type {
    NONE,           // 아무처리하지 않음.
	RETURN,         // 단순출력
    SUM,            // 요약출력	
	SUM_OR_FIRST,   // 단어가 없더라도 요약출력
};

// 필드명, 필드 속성
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
