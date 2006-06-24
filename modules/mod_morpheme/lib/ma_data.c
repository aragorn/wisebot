/* $Id$ */
/*
**  MA_DATA.C
**  2002. 01.  BY JaeBum, Kim.
*/
#include "common_core.h"
#include "memory.h"
#include "old_softbot.h"
#include "lb_std.h"
#include "ma_dic.h"
#include "ma_data.h"
#include <stdio.h>
#include <stdlib.h>

TDWORD p_nncg[2]  = { 0x00000001, 0x00000000 };
TDWORD p_nncv[2]  = { 0x00000002, 0x00000000 };
TDWORD p_nncj[2]  = { 0x00000004, 0x00000000 };
TDWORD p_nnb[2]   = { 0x00000008, 0x00000000 };
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
TDWORD p_xs[2]    = { 0x08003e00, 0x00000000 };	/*  접미사  */

TDWORD p_xpnn[2]  = { 0x00004000, 0x00000000 };
TDWORD p_xpnu[2]  = { 0x00008000, 0x00000000 };
TDWORD p_xp[2]    = { 0x0000c000, 0x00000000 };  /*  접두사  */

TDWORD p_ps[2]    = { 0x00010000, 0x00000000 };
TDWORD p_pc[2]    = { 0x00020000, 0x00000000 };
TDWORD p_po[2]    = { 0x00040000, 0x00000000 };
TDWORD p_pd[2]    = { 0x00080000, 0x00000000 };
TDWORD p_pa[2]    = { 0x00100000, 0x00000000 };
TDWORD p_pv[2]    = { 0x00200000, 0x00000000 };
TDWORD p_pn[2]    = { 0x00400000, 0x00000000 };
TDWORD p_px[2]    = { 0x00800000, 0x00000000 };
TDWORD p_p[2]     = { 0x00ff0000, 0x00000000 };	/*  조사  */

TDWORD p_da[2]    = { 0x01000000, 0x00000000 };
TDWORD p_di[2]    = { 0x02000000, 0x00000000 };
TDWORD p_du[2]    = { 0x04000000, 0x00000000 };
TDWORD p_d[2]     = { 0x07000000, 0x00000000 };	/*  관형사  */

TDWORD p_xsd[2]   = { 0x08000000, 0x00000000 };	/*  관형접사  */

TDWORD p_aa[2]    = { 0x10000000, 0x00000000 };
TDWORD p_ap[2]    = { 0x20000000, 0x00000000 };
TDWORD p_ai[2]    = { 0x40000000, 0x00000000 };
TDWORD p_ac[2]    = { 0x80000000, 0x00000000 };
TDWORD p_av[2]    = { 0x00000000, 0x00000001 };
TDWORD p_aj[2]    = { 0x00000000, 0x00000002 };
TDWORD p_a[2]     = { 0xf0000000, 0x00000013 };	/*  부사  */

TDWORD p_xsa[2]   = { 0x00000000, 0x00000004 };
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
TDWORD p_xsv[2]   = { 0x00000000, 0x00003c00 };	/*  용언접사  */

TDWORD p_eff[2]   = { 0x00000000, 0x00004000 };
TDWORD p_efc[2]   = { 0x00000000, 0x00008000 };
TDWORD p_efn[2]   = { 0x00000000, 0x00010000 };
TDWORD p_efd[2]   = { 0x00000000, 0x00020000 };
TDWORD p_efa[2]   = { 0x00000000, 0x00040000 };
TDWORD p_ep[2]    = { 0x00000000, 0x00080000 };
TDWORD p_e[2]	  = { 0x00000000, 0x000fc000 };	/*  OR  */

TDWORD p_un[2]    = { 0x00000000, 0x00100000 };	/*  추정체언  */
TDWORD p_uv[2]    = { 0x00000000, 0x00200000 };	/*  추정용언  */
TDWORD p_sevv[2]  = { 0x00000000, 0x00400000 };
TDWORD p_sevj[2]  = { 0x00000000, 0x00800000 };
TDWORD p_sho[2]   = { 0x00000000, 0x01000000 };
TDWORD p_sdb[2]   = { 0x00000000, 0x02000000 };
TDWORD p_sdn[2]   = { 0x00000000, 0x04000000 };
TDWORD p_sbxv[2]  = { 0x00000000, 0x08000000 };
TDWORD p_sbxj[2]  = { 0x00000000, 0x10000000 };
TDWORD p_scvv[2]  = { 0x00000000, 0x20000000 };
TDWORD p_scvj[2]  = { 0x00000000, 0x40000000 };

/*		p_n
TDWORD p_nncg[2]  = { 0x00000001, 0x00000000 };
TDWORD p_nncv[2]  = { 0x00000002, 0x00000000 };
TDWORD p_nncj[2]  = { 0x00000004, 0x00000000 };
TDWORD p_nnp[2]   = { 0x00000020, 0x00000000 };
TDWORD p_npp[2]	  = { 0x00000040, 0x00000000 };
*/
TDWORD p_n[2]     = { 0x00000067, 0x00000000 };
/*		p_nvj
TDWORD p_nncv[2]  = { 0x00000002, 0x00000000 };
TDWORD p_nncj[2]  = { 0x00000004, 0x00000000 };
*/
TDWORD p_nvj[2]   = { 0x00000006, 0x00000000 };
/*		p_ncp
TDWORD p_nncg[2]  = { 0x00000001, 0x00000000 };
TDWORD p_nnp[2]   = { 0x00000020, 0x00000000 };
*/
TDWORD p_ncp[2]   = { 0x00000021, 0x00000000 };

/*		p_v
TDWORD p_vv[2]    = { 0x00000000, 0x00000040 };
TDWORD p_vj[2]    = { 0x00000000, 0x00000100 };
*/
TDWORD p_v[2]     = { 0x00000000, 0x00000140 };

/*		p_pe
TDWORD p_e[2]	  = { 0x00000000, 0x000fc000 };
TDWORD p_p[2]     = { 0x00ff0000, 0x00000000 };
*/
TDWORD p_pe[2]    = { 0x00ff0000, 0x000fc000 };

/*		p_f
TDWORD p_xsnn[2]  = { 0x00000200, 0x00000000 };
TDWORD p_xsnnd[2] = { 0x00000400, 0x00000000 };
TDWORD p_xsnp[2]  = { 0x00000800, 0x00000000 };
TDWORD p_xsnu[2]  = { 0x00001000, 0x00000000 };
TDWORD p_xsnpl[2] = { 0x00002000, 0x00000000 };
TDWORD p_ps[2]    = { 0x00010000, 0x00000000 };
TDWORD p_pc[2]    = { 0x00020000, 0x00000000 };
TDWORD p_po[2]    = { 0x00040000, 0x00000000 };
TDWORD p_pd[2]    = { 0x00080000, 0x00000000 };
TDWORD p_pa[2]    = { 0x00100000, 0x00000000 };
TDWORD p_pv[2]    = { 0x00200000, 0x00000000 };
TDWORD p_pn[2]    = { 0x00400000, 0x00000000 };
TDWORD p_px[2]    = { 0x00800000, 0x00000000 };
TDWORD p_xsd[2]   = { 0x08000000, 0x00000000 };
TDWORD p_xsa[2]   = { 0x00000000, 0x00000004 };
TDWORD p_xsah[2]  = { 0x00000000, 0x00000008 };
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
TDWORD p_sevv[2]  = { 0x00000000, 0x00400000 };
TDWORD p_sevj[2]  = { 0x00000000, 0x00800000 };
*/
TDWORD p_f[2]     = { 0x08ff3e00, 0x00cffc0c };

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

TDWORD f_v_r	  = 0x00000024;
TDWORD f_v_i	  = 0x00000048;
TDWORD f_v_l	  = 0x00000090;
TDWORD f_p_pc	  = 0x00001c00;
TDWORD f_e_cc	  = 0x0007c000;
TDWORD f_e_pc	  = 0x00f00000;

/*
**		MEMBER
*/
/*********************************************************/
CBasePL	*m_pBasePL=0x00;

/*********************************************************/
void	MADinit(char *MAPH)
{
	FILE	*fpMA;

	if ( m_pBasePL != 0x00 )
		return;
	m_pBasePL = (CBasePL*)sb_calloc(1, sizeof(CBasePL));
	if ( m_pBasePL == 0x00 )
		STDErrSys("ERR lack memory for BasePL\n");

	fpMA = STDopenFP(MAPH, "rb");
	if ( fpMA == 0x00 )
		STDErrSys("ERR open %s\n", MAPH);

	STDseekFP(fpMA, 0L, SEEK_END);
	m_pBasePL->lAlocSZ = (TLONG)ftell(fpMA);
	STDseekFP(fpMA, 0L, SEEK_SET);

	m_pBasePL->BasePL = (char*)sb_calloc(1, m_pBasePL->lAlocSZ + 1);
	if ( m_pBasePL->BasePL == 0x00 )
		STDErrSys("ERR lack mem for ma.dic(%d)\n", m_pBasePL->lAlocSZ);

	if ( !STDreadFP(fpMA, m_pBasePL->BasePL, m_pBasePL->lAlocSZ, 1) )
		STDErrSys("ERR read %s\n", MAPH);

	STDcloseFP(fpMA);
	return;
}

/*********************************************************/
void	MADend()
{
	if ( m_pBasePL == 0x00 )
		return;

	if ( m_pBasePL->BasePL != 0x00 )
		sb_free(m_pBasePL->BasePL);
	m_pBasePL->BasePL = 0x00;

	sb_free(m_pBasePL);
	m_pBasePL = 0x00;
	return;
}

/*********************************************************/
TBOOL	MADgetPOS(TBYTE *Word, TDWORD *Pos)
{
	CBaseBK	*pBaseBK;
	TLONG	lBaseBK;
	TLONG	lBaseOff;
	TINT	i;

	lBaseBK = GET_BASE_BKNO(Word);
	lBaseOff = GET_BASE_OFF(lBaseBK);

	for ( ; ; )
	{
		pBaseBK = (CBaseBK*)(m_pBasePL->BasePL + lBaseOff);

		for ( i = 0; i < pBaseBK->shCntSL; i++ )
		{
			if ( strcmp(pBaseBK->aBaseSL[i].szWord, Word) == 0 )
				break;
		}

		if ( i < pBaseBK->shCntSL )
		{
			memcpy(Pos, pBaseBK->aBaseSL[i].POS, (sizeof(TDWORD) * 3));
			return 1;
		}

		if ( pBaseBK->lExtOff <= 0 )
			break;

		lBaseOff = pBaseBK->lExtOff;
	}

	memset(Pos, 0x00, (sizeof(TDWORD) * 3) );

	return 0;
}


/*
**  END MA_DATA.C
*/
