/* $Id$ */
#ifndef _MOD_VBM_H_
#define _MOD_VBM_H_

#include <pthread.h>
#include "softbot.h"
#include "hook.h"
#include "mod_api/vbm.h"

int VBM_init();
int VBM_initBuf(VariableBuffer *pVarBuf);
void VBM_freeBuf(VariableBuffer *pVarBuf);
int VBM_getSize(VariableBuffer *pVarBuf);
int VBM_get(VariableBuffer *pVarBuf,int offset, int size, void* pBuf);
int VBM_append(VariableBuffer *pVarBuf,int size, void* pBuf);
int VBM_appendBuf(VariableBuffer *pVarBufDest,
					VariableBuffer *pVarBufToAppend);
int VBM_print(FILE *fp, VariableBuffer *pVarBuf);

#endif

