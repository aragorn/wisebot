/**
 * Operator Modulization
 *
 * $Id$
 */

typedef int (*operator_func)(sb_stack_t *st, QueryNode *);

typedef struct {
	char op_str[MAX_QPP_OP_NUM][MAX_QPP_OP_STR];	/* oprator charactor or string */
	int oprand_num;					/* operand number, unary, binary, multi-nary(?) */
	operator_func func;				/* function to be called by operator */
	void *usr_ptr1;
	void *usr_ptr2;
} operator_t;

/*
���ο� ������ �߰�����

1) mod_qpp�� �������� LoadOperator�� �߰��Ѵ�.

ex) LoadOperator [operator name] [operator charactors] [default operand num]\
				 [.so filename] [operator function name]

2) operator function �� �ڵ��Ͽ� .so���Ϸ� �ۼ��Ѵ�.

��Ȫ~! ���� ����~~ ���� �̷��� moduleȭ �� �� �ִ°��̶� ���ΰ�..
*/

/*
���� mod_qp�� mod_qpp���� ��� �ٲپ�� �� ���ΰ�..

1) mod_qpp/tokenizer.c�� �����ڸ��� ���� �ڵ��Ǿ� �ִ� config function ��
LoadOperator�� ��� ��ü.

ex) set_op_and -> set_op
*/
void set_op(configValue v)
{
	int i=0;
	char op[MAX_QPP_OP_STR];
	int numOP = v.argNum;

	if (numOP > MAX_QPP_OP_NUM) {
		numOP = MAX_QPP_OP_NUM;
	}

	for (i=0; i<numOP; i++) {
		strncpy(m_op,v.argument[i],MAX_QPP_OP_STR);
		m_op[MAX_QPP_OP_STR-1] = '\0';

		addStopCharFromOperator(m_op);
	}
}

/*
2) operator_t *m_operators���� ���� �����. 
3) isOpAND, isOpOR���� �Լ����� ��� ����.. TokenObj�� �� OPERATOR ID��
currentOperator�� operator_t�迭������ idx���ڷ� ���Ѵ�. (��, ��ϵ� ����..)
4) mod_qpp/tokenizer.c�� m_eachOperator�� ����ü�� �����ϰ� isOperator�Լ��� ��ħ
5) mod_qp/mod_qp.c�� operate�Լ��� mod_qp_operators.c�� ����.
*/


/****************************** mod_qp_operator.c ****************************/

/* operator setting �Լ� �ۼ� */
void set_operator_func(int opid) {
}

/* operator function �ۼ� */
int operator(sb_stack_t *stack, QueryNode *node) {
}

void operate (sb_stack_t *stack, QueryNode *node) {
}
