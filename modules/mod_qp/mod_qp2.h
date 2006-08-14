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
    char name[SHORT_STRING_SIZE];
    int morpid;       // morp id
    int type;       // enum field_type
    int id;         // field id
} field_info_t;
///////////////////////////////////////////////////////////

#endif
