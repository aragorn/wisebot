/* $Id$ */
#ifndef _VBM_H_
#define _VBM_H_ 1

#include "softbot.h"

// FIXME: VBM_.. stuff must be changed?? 'cause this is api declaration file
#define VBM_UNDEFINED_SYSTEM_PARAMETER	(-10)
#define VBM_INSUFFICIENT_BUF_BLOCK		(-11)
#define VBM_FAIL_MEMORY_ALLOCATION		(-12)

typedef struct {
	int size;
	void* pFirst;
	void* pLast;
	int lastBlockUsedBytes;
} VariableBuffer;

SB_DECLARE_HOOK(int,buffer_initbuf,(VariableBuffer *pVarBuf))
SB_DECLARE_HOOK(void,buffer_freebuf,(VariableBuffer *pVarBuf))
SB_DECLARE_HOOK(int,buffer_getsize,(VariableBuffer *pVarBuf))
SB_DECLARE_HOOK(int,buffer_get,(VariableBuffer *pVarBuf,int offset, int size, void* pBuf))
SB_DECLARE_HOOK(int,buffer_append,(VariableBuffer *pVarBuf,int size, void* pBuf))
SB_DECLARE_HOOK(int,buffer_appendbuf, \
		(VariableBuffer *pVarBufDest, VariableBuffer *pVarBufToAppend))
SB_DECLARE_HOOK(int,buffer_print,(FILE *fp, VariableBuffer *pVarBuf))
SB_DECLARE_HOOK(int,buffer_str,(char *result, VariableBuffer *pVarBuf))

#endif
