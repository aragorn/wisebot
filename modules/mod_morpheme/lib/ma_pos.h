/* $Id$ */
/*
**  MA_POS.H
**  2002.01.  BY JaeBum, Kim.
*/
#ifndef _HD_POS_
#define _HD_POS_

#include "old_softbot.h"

#define MAX_POS_NUM		(96)
#define MAX_POS_LEN		(7)

char	m_PosTB[MAX_POS_NUM][MAX_POS_LEN] = {
"NNCG", /* 	:체언(N):명사(N):보통(C):일반(G)  */
"NNCV", /* 	:체언(N):명사(N):보통(C):동사(V)  */
"NNCJ",	/* 	:체언(N):명사(N):보통(C):형용사(J)  */
"NNB",	/* 	:체언(N):명사(N):의존(B)		*/
"NNBU",	/* 	:체언(N):명사(N):의존(B):단위(U)	*/
"NNP",	/* 	:체언(N):명사(N):고유(P)		*/
"NPP",	/* 	:체언(N):대명사(P):인칭(P)		*/
"NPI",	/* 	:체언(N):대명사(P):지시(I)		*/
"NU",	/* 	:체언(N):수사(U)				*/
"XSNN",	/* 	:접사(X):접미(S):체언(N):명사(N)	*/
"XSNND",	/* 	:접사(X):접미(S):체언(N):명사(N):관형(D)-적	*/
"XSNP",	/* 	:접사(X):접미(S):체언(N):대명사(P)	*/
"XSNU",	/* 	:접사(X):접미(S):체언(N):수사(U)	*/
"XSNPL",	/* 	:접사(X):접미(S):체언(N):복수(PL)-들	*/
"XPNN",	/* 	:접사(X):접두(P):체언(N):명사(N)	*/
"XPNU",	/* 	:접사(X):접두(P):체언(N):수사(U)	*/
"PS",	/* 	:조사(P):주격(S)		*/
"PC",	/* 	:조사(P):보격(C)		*/
"PO",	/* 	:조사(P):목적격(O)		*/
"PD",	/* 	:조사(P):관형격(D)		*/
"PA",	/* 	:조사(P):부사격(A)		*/
"PV",	/* 	:조사(P):호격(V)		*/
"PN",	/* 	:조사(P):접속(N)		*/
"PX",	/* 	:조사(P):보조(X)		*/
"DA",	/* 	:관형사(D):성상(A)		*/
"DI",	/* 	:관형사(D):지시(I)		*/
"DU",	/* 	:관형사(D):수(U)		*/
"XSD",	/* 	:접사(X):접미(S):관형사(D)-적	*/
"AA",	/* 	:부사(A):성상(A)		*/
"AP",	/* 	:부사(A):서술(P)		*/
"AI",	/* 	:부사(A):지시(I)		*/
"AC",	/* 	:부사(A):접속(C)		*/
"AV",	/* 	:부사(A):동사(V)		*/
"AJ",	/* 	:부사(A):형용사(J)		*/
"XSA",	/* 	:접사(X):접미(S):부사(A)	*/
"XSAH",	/* 	:접사(X):접미(S):부사(A):히(H)-히	*/
"C",	/* 	:감탄사(C)		*/
"I",	/* 	:서술격조사(I)		*/
"VV",	/* 	:용언(V):동사(V)	*/
"VVX",	/* 	:용언(V):동사(V):보조(X)	*/
"VJ",	/* 	:용언(V):형용사(J)			*/
"VJX",	/* 	:용언(V):형용사(J):보조(X)	*/
"XSVV",	/* 	:접사(X):접미(S):용언화(V):동사(V)		*/
"XSVJ",	/* 	:접사(X):접미(S):용언화(V):형동사(J)	*/
"XSVJD",	/* 	:접사(X):접미(S):용언화(V):형동사(J):답(D)-답	*/
"XSVJB",	/* 	:접사(X):접미(S):용언화(V):형동사(J):기타(B)-롭,스럽	*/
"EFF",	/* 	:어미(E):어말(F):종결(F)	*/
"EFC",	/* 	:어미(E):어말(F):연결(C)	*/
"EFN",	/* 	:어미(E):어말(F):명사(N)	*/
"EFD",	/* 	:어미(E):어말(F):관형(D)	*/
"EFA",	/* 	:어미(E):어말(F):부사(A)	*/
"EP",	/* 	:어미(E):선어말(P)			*/
"NN?",	/*	:체언(N):명사(N):추정(?)	*/
"V?",	/* 	:용언(V):추정(?)			*/
"SEVV",	/*	:어미+보조동사				*/
"SEVJ",	/*	:어미+보조형용사			*/
"SHO",	/*	:'하'축약형-생각다,생각지,...	*/
"SDB",	/*	:관형사(D)+의존명사(B)-이층,삼학년,...	*/
"SDN",	/*	:1음절관형어(D)+1음절명사(N)-그때,그곳,큰것,이말,저말,한잎,두잎,...		*/
"SBXV",	/* 	:의존명사(B)+보조동사(XV)-듯하,척하,...		*/
"SBXJ",	/* 	:의존명사(B)+보조형용사(XJ)-듯싶,...		*/
"SCVV",	/* 	:복합동사:명사+동사							*/
"SCVJ",	/* 	:복합형용사:명사+형용사						*/
"64",	/* 	:예약										*/
"N.IR",	/*	:불규칙체언									*/
"NN.CM",	/*	:합성명사								*/
"VV.R",	/* 	:규칙활용동사								*/
"VV.I",	/* 	:불규칙활용동사(러불규칙외)					*/
"VV.L",	/* 	:러불규칙활용동사							*/
"VJ.R",	/* 	:규칙활용형용사								*/
"VJ.I",	/* 	:불규칙활용형용사(러불규칙외)				*/
"VJ.L",	/* 	:러불규칙활용형용사							*/
"V.NC",	/*	:비축약용언									*/
"P.R.I",	/*	:서술격조사결합조사						*/
"P.V",	/* 	:무종성결합조사								*/
"P.C",	/* 	:유종성결합조사								*/
"P.L",	/* 	:ㄹ종성결합조사								*/
"P.IR",	/* 	:불규칙조사									*/
"E.L.V",	/* 	:동사결합어미		*/
"E.L.J",	/* 	:형용사결합어미		*/
"E.L.I",	/* 	:서술격조사결합어미	*/
"E.L.IX",	/* 	:서술격조사생략어미	*/
"E.L.P",	/* 	:선어말어미결합어미	*/
"E.R.P",	/* 	:조사결합어미		*/
"E.V",	/* 	:무종성결합어미		*/
"E.C",	/* 	:유종성결합어미		*/
"E.S",	/* 	:ㅆ,ㅄ종성결합어미	*/
"E.L",	/* 	:ㄹ종성결합어미		*/
"E.LX",	/* 	:ㄹ탈락어미			*/
"E.HX",	/* 	:ㅎ탈락어미			*/
"E.HC",	/* 	:하축약어미			*/
"E.IR",	/* 	:불규칙어미			*/
"E.P",	/*	:양성어미			*/
"E.N",	/*	:음성어미			*/
"95",	/* 	:예약				*/
"96",	/* 	:예약				*/
};

#define POSM_GET_INT(a, b)\
{\
	for ( b = 0; b < MAX_POS_NUM; b++ )\
	{\
		if ( strcmp(a, m_PosTB[b]) == 0 )\
			break;\
	}\
	b = ((b > MAX_POS_NUM)? MAX_POS_NUM : b);\
}

TDWORD p_nncg[2]  = { 0x00000001, 0x00000000 };
TDWORD p_nncv[2]  = { 0x00000002, 0x00000000 };
TDWORD p_nncj[2]  = { 0x00000004, 0x00000000 };
TDWORD p_nnb[2]   = { 0x00000018, 0x00000000 };
TDWORD p_nnbu[2]  = { 0x00000010, 0x00000000 };
TDWORD p_nnp[2]   = { 0x00000020, 0x00000000 };
TDWORD p_npp[2]	  = { 0x00000040, 0x00000000 };
TDWORD p_npi[2]   = { 0x00000080, 0x00000000 };
TDWORD p_nu[2]    = { 0x00000100, 0x00000000 };
TDWORD p_xsnn[2]  = { 0x00000200, 0x00000000 };
TDWORD p_xsnnd[2] = { 0x00000400, 0x00000000 };
TDWORD p_xsnp[2]  = { 0x00000800, 0x00000000 };
TDWORD p_xsnu[2]  = { 0x00001000, 0x00000000 };
TDWORD p_xsnpl[2] = { 0x00002000, 0x00000000 };
TDWORD p_xpnn[2]  = { 0x00004000, 0x00000000 };
TDWORD p_xpnu[2]  = { 0x00008000, 0x00000000 };
TDWORD p_xp[2]    = { 0x0000c000, 0x00000000 };  /*  OR  */
TDWORD p_ps[2]    = { 0x00010000, 0x00000000 };
TDWORD p_pc[2]    = { 0x00020000, 0x00000000 };
TDWORD p_po[2]    = { 0x00040000, 0x00000000 };
TDWORD p_pd[2]    = { 0x00080000, 0x00000000 };
TDWORD p_pa[2]    = { 0x00100000, 0x00000000 };
TDWORD p_pv[2]    = { 0x00200000, 0x00000000 };
TDWORD p_pn[2]    = { 0x00400000, 0x00000000 };
TDWORD p_px[2]    = { 0x00800000, 0x00000000 };
TDWORD p_p[2]     = { 0x00ff0000, 0x00000000 };	/*  OR  */
TDWORD p_da[2]    = { 0x01000000, 0x00000000 };
TDWORD p_di[2]    = { 0x02000000, 0x00000000 };
TDWORD p_du[2]    = { 0x04000000, 0x00000000 };
TDWORD p_d[2]     = { 0x0f000000, 0x00000000 };	/*  OR  */
TDWORD p_xsd[2]   = { 0x08000000, 0x00000000 };
TDWORD p_aa[2]    = { 0x10000000, 0x00000000 };
TDWORD p_ap[2]    = { 0x20000000, 0x00000000 };
TDWORD p_ai[2]    = { 0x40000000, 0x00000000 };
TDWORD p_ac[2]    = { 0x80000000, 0x00000000 };
TDWORD p_av[2]    = { 0x00000000, 0x00000001 };
TDWORD p_aj[2]    = { 0x00000000, 0x00000002 };
TDWORD p_a[2]     = { 0xf0000000, 0x00000013 };	/*  OR  */
TDWORD p_xsa[2]   = { 0x00000000, 0x0000000c };	/*  OR  */
TDWORD p_xsah[2]  = { 0x00000000, 0x00000008 };
TDWORD p_c[2]     = { 0x00000000, 0x00000010 };
TDWORD p_i[2]     = { 0x00000000, 0x00000020 };
TDWORD p_vv[2]    = { 0x00000000, 0x00000040 };
TDWORD p_vvx[2]   = { 0x00000000, 0x00000080 };
TDWORD p_vj[2]    = { 0x00000000, 0x00000100 };
TDWORD p_vjx[2]   = { 0x00000000, 0x00000200 };
TDWORD p_xsvv[2]  = { 0x00000000, 0x00000400 };
TDWORD p_xsvj[2]  = { 0x00000000, 0x00000800 };
TDWORD p_xsvjd[2] = { 0x00000000, 0x00001000 };
TDWORD p_xsvjb[2] = { 0x00000000, 0x00002000 };
TDWORD p_eff[2]   = { 0x00000000, 0x00004000 };
TDWORD p_efc[2]   = { 0x00000000, 0x00008000 };
TDWORD p_efn[2]   = { 0x00000000, 0x00010000 };
TDWORD p_efd[2]   = { 0x00000000, 0x00020000 };
TDWORD p_efa[2]   = { 0x00000000, 0x00040000 };
TDWORD p_ep[2]    = { 0x00000000, 0x00080000 };
TDWORD p_e[2]	  = { 0x00000000, 0x000fc000 };	/*  OR  */

TDWORD p_sevv[2]  = { 0x00000000, 0x00400000 };
TDWORD p_sevj[2]  = { 0x00000000, 0x00800000 };
TDWORD p_sho[2]   = { 0x00000000, 0x01000000 };
TDWORD p_sdb[2]   = { 0x00000000, 0x02000000 };
TDWORD p_sdn[2]   = { 0x00000000, 0x04000000 };
TDWORD p_sbxv[2]  = { 0x00000000, 0x08000000 };
TDWORD p_sbxj[2]  = { 0x00000000, 0x10000000 };
TDWORD p_scvv[2]  = { 0x00000000, 0x20000000 };
TDWORD p_scvj[2]  = { 0x00000000, 0x40000000 };

TDWORD f_n_ir	  = 0x00000001;
TDWORD f_n_cm	  = 0x00000002;
TDWORD f_vv_r	  = 0x00000004;
TDWORD f_vv_i	  = 0x00000008;
TDWORD f_vv_l	  = 0x00000010;
TDWORD f_vj_r	  = 0x00000020;
TDWORD f_vj_i	  = 0x00000040;
TDWORD f_vj_l	  = 0x00000080;
TDWORD f_v_nc	  = 0x00000100;
TDWORD f_p_r_i    = 0x00000200;
TDWORD f_p_v	  = 0x00000400;
TDWORD f_p_c	  = 0x00000800;
TDWORD f_p_l	  = 0x00001000;
TDWORD f_p_ir	  = 0x00002000;
TDWORD f_e_l_v    = 0x00004000;
TDWORD f_e_l_j    = 0x00008000;
TDWORD f_e_l_i    = 0x00010000;
TDWORD f_e_l_ix   = 0x00020000;
TDWORD f_e_l_p    = 0x00040000;
TDWORD f_e_r_p    = 0x00080000;
TDWORD f_e_v	  = 0x00100000;
TDWORD f_e_c	  = 0x00200000;
TDWORD f_e_s	  = 0x00400000;
TDWORD f_e_l	  = 0x00800000;
TDWORD f_e_lx	  = 0x01000000;
TDWORD f_e_hx	  = 0x02000000;
TDWORD f_e_hc	  = 0x04000000;
TDWORD f_e_ir	  = 0x08000000;
TDWORD f_e_p	  = 0x10000000;
TDWORD f_e_n	  = 0x20000000;
TDWORD f_inf	  = 0x40000000;
TDWORD f_def	  = 0x80000000;
TDWORD f_noise	  = 0x40000000;
TDWORD f_user     = 0x80000000;

TDWORD p_un[2]    = { 0x00000000, 0x00100000 };
TDWORD p_n[2]	  = { 0x000001ff, 0x00100000 };
TDWORD p_n2[2]    = { 0x00000021, 0x00000000 };
TDWORD p_xsn[2]   = { 0x08003e00, 0x00000000 };
TDWORD p_xsv[2]   = { 0x00000000, 0x00003c00 };
TDWORD p_xs[2]    = { 0x08003e00, 0x00003c0c };
TDWORD p_uv[2]    = { 0x00000000, 0x00200000 };
TDWORD p_v[2]     = { 0x00000000, 0x002003c0 };
TDWORD p_s[2]     = { 0x00000000, 0x7fc00000 };

TDWORD f_v_r	  = 0x00000024;
TDWORD f_v_i	  = 0x00000048;
TDWORD f_v_l	  = 0x00000090;
TDWORD f_p_pc	  = 0x00001c00;
TDWORD f_e_cc	  = 0x0007c000;
TDWORD f_e_pc	  = 0x00f00000;

#define IS_N(a,b)		( ((a) & p_n[0]) || ((b) & p_n[1]) )
#define IS_V(a,b)		( ((a) & p_v[0]) || ((b) & p_v[1]) )
#define IS_A(a,b)		( ((a) & p_a[0]) || ((b) & p_a[1]) )
#define IS_I(a,b)		( ((a) & p_i[0]) || ((b) & p_i[1]) )

#define IS_P(a,b)		( ((a) & p_p[0]) || ((b) & p_p[1]) )
#define IS_E(a,b)		( ((a) & p_e[0]) || ((b) & p_e[1]) )
#define IS_XS(a,b)		( ((a) & p_xs[0]) || ((b) & p_xs[1]) )

#define IS_NNCM(a)		( ((a) & f_n_cm) )

#define IS_NV(a,b)		( ((a) & p_n[0]) || ((b) & p_n[1]) || ((a) & p_v[0]) || ((b) & p_v[1]) )
#define IS_N2(a,b)		( ((a) & p_n2[0]) || ((b) & p_n2[1]) )
#define IS_NNB(a,b)	( ((a) & p_nnb[0]) || ((b) & p_nnb[1]) )
#define IS_NVJ(a,b)	( ((a) & p_nncv[0]) || ((b) & p_nncv[1]) || ((a) & p_nncj[0]) || ((b) & p_nncj[1]) )
#define IS_NNBU(a,b)	( ((a) & p_nnbu[0]) || ((b) & p_nnbu[1]) )
#define IS_NNP(a,b)	( ((a) & p_nnp[0]) || ((b) & p_nnp[1]) )


#endif	/*  _MA_POS_  */

/*
**  END MA_POS.H
*/
