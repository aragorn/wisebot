/* $Id$ */
/*
**  MA_JAMO.H
**  2002. 01.  BY JaeBum, Kim.
*/
#ifndef _MA_JAMO_
#define _MA_JAMO_

#include "old_softbot.h"

/* Phoneme-type hangul code */
typedef struct tagJamo
{
    TBYTE	i;	/* phoneme : initial */
    TBYTE	m;	/* phoneme : medial */
    TBYTE	f;	/* phoneme : final */
} CJamo;

/* initial phoneme : �ʼ� */
#define PHI_FIL	(unsigned char)1	/* �ʼ� ä�� �ڵ� */
#define PHI_G	(unsigned char)2	/* �ʼ� �� */
#define PHI_GG	(unsigned char)3	/* �ʼ� �� */
#define PHI_N	(unsigned char)4	/* �ʼ� �� */
#define PHI_D	(unsigned char)5	/* �ʼ� �� */
#define PHI_DD	(unsigned char)6	/* �ʼ� �� */
#define PHI_R	(unsigned char)7	/* �ʼ� �� */
#define PHI_M	(unsigned char)8	/* �ʼ� �� */
#define PHI_B	(unsigned char)9	/* �ʼ� �� */
#define PHI_BB	(unsigned char)10	/* �ʼ� �� */
#define PHI_S	(unsigned char)11	/* �ʼ� �� */
#define PHI_SS	(unsigned char)12	/* �ʼ� �� */
#define PHI_NG	(unsigned char)13	/* �ʼ� �� */
#define PHI_J	(unsigned char)14	/* �ʼ� �� */
#define PHI_JJ	(unsigned char)15	/* �ʼ� �� */
#define PHI_CH	(unsigned char)16	/* �ʼ� �� */
#define PHI_K	(unsigned char)17	/* �ʼ� �� */
#define PHI_T	(unsigned char)18	/* �ʼ� �� */
#define PHI_P	(unsigned char)19	/* �ʼ� �� */
#define PHI_H	(unsigned char)20	/* �ʼ� �� */

/* medial phoneme : �߼� */
#define PHM_FIL	(unsigned char)2	/* �߼� ä�� �ڵ� */
#define PHM_A	(unsigned char)3	/* �߼� �� */
#define PHM_AI	(unsigned char)4	/* �߼� �� */
#define PHM_YA	(unsigned char)5	/* �߼� �� */
#define PHM_YAI	(unsigned char)6	/* �߼� �� */
#define PHM_EO	(unsigned char)7	/* �߼� �� */
#define PHM_EOI	(unsigned char)10	/* �߼� �� */
#define PHM_YEO	(unsigned char)11	/* �߼� �� */
#define PHM_YEOI	(unsigned char)12	/* �߼� �� */
#define PHM_O	(unsigned char)13	/* �߼� �� */
#define PHM_OA	(unsigned char)14	/* �߼� �� */
#define PHM_OAI	(unsigned char)15	/* �߼� �� */
#define PHM_OI	(unsigned char)18	/* �߼� �� */
#define PHM_YO	(unsigned char)19	/* �߼� �� */
#define PHM_U	(unsigned char)20	/* �߼� �� */
#define PHM_UEO	(unsigned char)21	/* �߼� �� */
#define PHM_UEOI	(unsigned char)22	/* �߼� �� */
#define PHM_UI	(unsigned char)23	/* �߼� �� */
#define PHM_YU	(unsigned char)26	/* �߼� �� */
#define PHM_EU	(unsigned char)27	/* �߼� �� */
#define PHM_EUI	(unsigned char)28	/* �߼� �� */
#define PHM_I	(unsigned char)29	/* �߼� �� */

/* final phoneme : ���� */
#define PHF_FIL	(unsigned char)1	/* ���� ä�� �ڵ� */
#define PHF_G	(unsigned char)2	/* ���� �ԤԤԤ� */
#define PHF_GG	(unsigned char)3	/* ���� �ԤԤԤ� */
#define PHF_GS	(unsigned char)4	/* ���� �ԤԤԤ� */
#define PHF_N	(unsigned char)5	/* ���� �ԤԤԤ� */
#define PHF_NJ	(unsigned char)6	/* ���� �ԤԤԤ� */
#define PHF_NH	(unsigned char)7	/* ���� �ԤԤԤ� */
#define PHF_D	(unsigned char)8	/* ���� �ԤԤԤ� */
#define PHF_R	(unsigned char)9	/* ���� �ԤԤԤ� */
#define PHF_RG	(unsigned char)10	/* ���� �ԤԤԤ� */
#define PHF_RM	(unsigned char)11	/* ���� �ԤԤԤ� */
#define PHF_RB	(unsigned char)12	/* ���� �ԤԤԤ� */
#define PHF_RS	(unsigned char)13	/* ���� �ԤԤԤ� */
#define PHF_RP	(unsigned char)14	/* ���� �ԤԤԤ� */
#define PHF_RH	(unsigned char)15	/* ���� �ԤԤԤ� */
#define PHF_RT	(unsigned char)16	/* ���� �ԤԤԤ� */
#define PHF_M	(unsigned char)17	/* ���� �ԤԤԤ� */
#define PHF_B	(unsigned char)19	/* ���� �ԤԤԤ� */
#define PHF_BS	(unsigned char)20	/* ���� �ԤԤԤ� */
#define PHF_S	(unsigned char)21	/* ���� �ԤԤԤ� */
#define PHF_SS	(unsigned char)22	/* ���� �ԤԤԤ� */
#define PHF_NG	(unsigned char)23	/* ���� �ԤԤԤ� */
#define PHF_J	(unsigned char)24	/* ���� �ԤԤԤ� */
#define PHF_CH	(unsigned char)25	/* ���� �ԤԤԤ� */
#define PHF_K	(unsigned char)26	/* ���� �ԤԤԤ� */
#define PHF_T	(unsigned char)27	/* ���� �ԤԤԤ� */
#define PHF_P	(unsigned char)28	/* ���� �ԤԤԤ� */
#define PHF_H	(unsigned char)29	/* ���� �ԤԤԤ� */

#endif	/*  _MA_JAMO_  */

/*
**  END MA_JAMO.H
*/
