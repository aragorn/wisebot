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
새로운 연산자 추가과정

1) mod_qpp의 설정에서 LoadOperator를 추가한다.

ex) LoadOperator [operator name] [operator charactors] [default operand num]\
				 [.so filename] [operator function name]

2) operator function 을 코딩하여 .so파일로 작성한다.

오홋~! 뎁따 간단~~ 정말 이렇게 module화 할 수 있는것이란 말인가..
*/

/*
현재 mod_qp와 mod_qpp에서 어떻게 바꾸어야 할 것인가..

1) mod_qpp/tokenizer.c에 연산자마다 각각 코딩되어 있는 config function 을
LoadOperator로 모두 대체.

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
2) operator_t *m_operators전역 수를 만든다. 
3) isOpAND, isOpOR등의 함수들을 모두 통합.. TokenObj에 들어갈 OPERATOR ID인
currentOperator는 operator_t배열에서의 idx숫자로 정한다. (즉, 등록된 순서..)
4) mod_qpp/tokenizer.c의 m_eachOperator란 구조체를 삭제하고 isOperator함수를 고침
5) mod_qp/mod_qp.c의 operate함수를 mod_qp_operators.c로 뺀다.
*/


/****************************** mod_qp_operator.c ****************************/

/* operator setting 함수 작성 */
void set_operator_func(int opid) {
}

/* operator function 작성 */
int operator(sb_stack_t *stack, QueryNode *node) {
}

void operate (sb_stack_t *stack, QueryNode *node) {
}
