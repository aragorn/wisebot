/* $Id$ */
/*
**  MA_DATA.H
**  2002. 01.  BY JaeBum, Kim.
*/
#ifndef _MA_DATA_
#define _MA_DATA_

#include "old_softbot.h"

#define IS_N(a,b)		( ((a) & p_n[0]) || ((b) & p_n[1]) )
#define IS_NVJ(a,b)		( ((a) & p_nvj[0]) || ((b) & p_nvj[1]) )
#define IS_NCP(a,b)     ( ((a) & p_ncp[0]) || ((b) & p_ncp[1]) )
#define IS_NNP(a,b)		( ((a) & p_nnp[0]) || ((b) & p_nnp[1]) )
#define IS_V(a,b)		( ((a) & p_v[0]) || ((b) & p_v[1]) )
#define IS_F(a,b)		( ((a) & p_f[0]) || ((b) & p_f[1]) )
#define IS_P(a,b)		( ((a) & p_p[0]) || ((b) & p_p[1]) )
#define IS_E(a,b)		( ((a) & p_e[0]) || ((b) & p_e[1]) )
#define IS_PE(a,b)		( ((a) & p_pe[0]) || ((b) & p_pe[1]) )
#define IS_XS(a,b)		( ((a) & p_xs[0]) || ((b) & p_xs[1]) )
#define IS_XP(a,b)		( ((a) & p_xp[0]) || ((b) & p_xp[1]) )

#define IS_NOISE(a)		( ((a) & f_noise) )
#define IS_USER(a)		( ((a) & f_user) )

#define IS_NNCM(a)		( ((a) & f_n_cm) )

#ifdef _NOT_YET_
#define IS_A(a,b)		( ((a) & p_a[0]) || ((b) & p_a[1]) )
#define IS_I(a,b)		( ((a) & p_i[0]) || ((b) & p_i[1]) )

#define IS_NV(a,b)		( ((a) & p_n[0]) || ((b) & p_n[1]) || ((a) & p_v[0]) || ((b) & p_v[1]) )
#define IS_N2(a,b)		( ((a) & p_n2[0]) || ((b) & p_n2[1]) )
#define IS_NNB(a,b)	( ((a) & p_nnb[0]) || ((b) & p_nnb[1]) )
#define IS_NVJ(a,b)	( ((a) & p_nncv[0]) || ((b) & p_nncv[1]) || ((a) & p_nncj[0]) || ((b) & p_nncj[1]) )
#define IS_NNBU(a,b)	( ((a) & p_nnbu[0]) || ((b) & p_nnbu[1]) )
#define IS_NNP(a,b)	( ((a) & p_nnp[0]) || ((b) & p_nnp[1]) )
#endif	/*  NOT_YET  */

#endif	/*  _MA_DATA_  */

extern TDWORD p_n[2];
extern TDWORD p_nvj[2];
extern TDWORD p_ncp[2];
extern TDWORD p_nnp[2];
extern TDWORD p_v[2];
extern TDWORD p_f[2];
extern TDWORD p_p[2];
extern TDWORD p_e[2];
extern TDWORD p_pe[2];
extern TDWORD p_xs[2];
extern TDWORD p_xp[2];
extern TDWORD f_noise;
extern TDWORD f_user;
extern TDWORD f_n_cm;


/*********************************************************/
void	MADinit(char *MAPH);
void	MADend();
TBOOL	MADgetPOS(TBYTE *Word, TDWORD *Pos);

/*
**  END MA_DATA.H
*/
