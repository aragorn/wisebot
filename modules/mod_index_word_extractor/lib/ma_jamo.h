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

/* initial phoneme : 檬己 */
#define PHI_FIL	(unsigned char)1	/* 檬己 盲框 内靛 */
#define PHI_G	(unsigned char)2	/* 檬己 ぁ */
#define PHI_GG	(unsigned char)3	/* 檬己 あ */
#define PHI_N	(unsigned char)4	/* 檬己 い */
#define PHI_D	(unsigned char)5	/* 檬己 ぇ */
#define PHI_DD	(unsigned char)6	/* 檬己 え */
#define PHI_R	(unsigned char)7	/* 檬己 ぉ */
#define PHI_M	(unsigned char)8	/* 檬己 け */
#define PHI_B	(unsigned char)9	/* 檬己 げ */
#define PHI_BB	(unsigned char)10	/* 檬己 こ */
#define PHI_S	(unsigned char)11	/* 檬己 さ */
#define PHI_SS	(unsigned char)12	/* 檬己 ざ */
#define PHI_NG	(unsigned char)13	/* 檬己 し */
#define PHI_J	(unsigned char)14	/* 檬己 じ */
#define PHI_JJ	(unsigned char)15	/* 檬己 す */
#define PHI_CH	(unsigned char)16	/* 檬己 ず */
#define PHI_K	(unsigned char)17	/* 檬己 せ */
#define PHI_T	(unsigned char)18	/* 檬己 ぜ */
#define PHI_P	(unsigned char)19	/* 檬己 そ */
#define PHI_H	(unsigned char)20	/* 檬己 ぞ */

/* medial phoneme : 吝己 */
#define PHM_FIL	(unsigned char)2	/* 吝己 盲框 内靛 */
#define PHM_A	(unsigned char)3	/* 吝己 た */
#define PHM_AI	(unsigned char)4	/* 吝己 だ */
#define PHM_YA	(unsigned char)5	/* 吝己 ち */
#define PHM_YAI	(unsigned char)6	/* 吝己 ぢ */
#define PHM_EO	(unsigned char)7	/* 吝己 っ */
#define PHM_EOI	(unsigned char)10	/* 吝己 つ */
#define PHM_YEO	(unsigned char)11	/* 吝己 づ */
#define PHM_YEOI	(unsigned char)12	/* 吝己 て */
#define PHM_O	(unsigned char)13	/* 吝己 で */
#define PHM_OA	(unsigned char)14	/* 吝己 と */
#define PHM_OAI	(unsigned char)15	/* 吝己 ど */
#define PHM_OI	(unsigned char)18	/* 吝己 な */
#define PHM_YO	(unsigned char)19	/* 吝己 に */
#define PHM_U	(unsigned char)20	/* 吝己 ぬ */
#define PHM_UEO	(unsigned char)21	/* 吝己 ね */
#define PHM_UEOI	(unsigned char)22	/* 吝己 の */
#define PHM_UI	(unsigned char)23	/* 吝己 は */
#define PHM_YU	(unsigned char)26	/* 吝己 ば */
#define PHM_EU	(unsigned char)27	/* 吝己 ぱ */
#define PHM_EUI	(unsigned char)28	/* 吝己 ひ */
#define PHM_I	(unsigned char)29	/* 吝己 び */

/* final phoneme : 辆己 */
#define PHF_FIL	(unsigned char)1	/* 辆己 盲框 内靛 */
#define PHF_G	(unsigned char)2	/* 辆己 ぴぴぴぁ */
#define PHF_GG	(unsigned char)3	/* 辆己 ぴぴぴあ */
#define PHF_GS	(unsigned char)4	/* 辆己 ぴぴぴぃ */
#define PHF_N	(unsigned char)5	/* 辆己 ぴぴぴい */
#define PHF_NJ	(unsigned char)6	/* 辆己 ぴぴぴぅ */
#define PHF_NH	(unsigned char)7	/* 辆己 ぴぴぴう */
#define PHF_D	(unsigned char)8	/* 辆己 ぴぴぴぇ */
#define PHF_R	(unsigned char)9	/* 辆己 ぴぴぴぉ */
#define PHF_RG	(unsigned char)10	/* 辆己 ぴぴぴお */
#define PHF_RM	(unsigned char)11	/* 辆己 ぴぴぴか */
#define PHF_RB	(unsigned char)12	/* 辆己 ぴぴぴが */
#define PHF_RS	(unsigned char)13	/* 辆己 ぴぴぴき */
#define PHF_RP	(unsigned char)14	/* 辆己 ぴぴぴく */
#define PHF_RH	(unsigned char)15	/* 辆己 ぴぴぴぐ */
#define PHF_RT	(unsigned char)16	/* 辆己 ぴぴぴぎ */
#define PHF_M	(unsigned char)17	/* 辆己 ぴぴぴけ */
#define PHF_B	(unsigned char)19	/* 辆己 ぴぴぴげ */
#define PHF_BS	(unsigned char)20	/* 辆己 ぴぴぴご */
#define PHF_S	(unsigned char)21	/* 辆己 ぴぴぴさ */
#define PHF_SS	(unsigned char)22	/* 辆己 ぴぴぴざ */
#define PHF_NG	(unsigned char)23	/* 辆己 ぴぴぴし */
#define PHF_J	(unsigned char)24	/* 辆己 ぴぴぴじ */
#define PHF_CH	(unsigned char)25	/* 辆己 ぴぴぴず */
#define PHF_K	(unsigned char)26	/* 辆己 ぴぴぴせ */
#define PHF_T	(unsigned char)27	/* 辆己 ぴぴぴぜ */
#define PHF_P	(unsigned char)28	/* 辆己 ぴぴぴそ */
#define PHF_H	(unsigned char)29	/* 辆己 ぴぴぴぞ */

#endif	/*  _MA_JAMO_  */

/*
**  END MA_JAMO.H
*/
