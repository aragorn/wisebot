/* $Id$ */
#ifndef MOD_QP2_H
#define MOD_QP2_H 1

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
    int id;         /* field id */
    char name[SHORT_STRING_SIZE];
    int index;      /* 1 for yes, 0 for no */
    int indexer_morpid;
    int qpp_morpid;
    int type;       // enum field_type
} field_info_t;
///////////////////////////////////////////////////////////

#endif
