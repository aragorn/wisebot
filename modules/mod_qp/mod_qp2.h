/* $Id$ */
#ifndef MOD_QP2_H
#define MOD_QP2_H 1

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
    int id;         /* field id */
    char name[SHORT_STRING_SIZE];
    int index;      /* 1 for yes, 0 for no */
    int indexer_morpid;
    int qpp_morpid;
    int type;       // enum field_type
} field_info_t;
///////////////////////////////////////////////////////////

#endif
