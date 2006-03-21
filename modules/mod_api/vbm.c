/* $Id$ */
#include "common_core.h"
#include "vbm.h"

HOOK_STRUCT(
	HOOK_LINK(buffer_initbuf)
	HOOK_LINK(buffer_freebuf)
	HOOK_LINK(buffer_get)
	HOOK_LINK(buffer_getsize) /* FIXME: MACRO로 하는게 낫지 않을까? */
	HOOK_LINK(buffer_append)
	HOOK_LINK(buffer_appendbuf)
	HOOK_LINK(buffer_print)
	HOOK_LINK(buffer_str)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_initbuf,(VariableBuffer *pVarBuf),(pVarBuf),0)
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(buffer_freebuf,(VariableBuffer *pVarBuf),(pVarBuf))
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_getsize,(VariableBuffer *pVarBuf),(pVarBuf),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_get, \
(VariableBuffer *pVarBuf,int offset, int size, void* pBuf),(pVarBuf,offset,size,pBuf),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_append, \
(VariableBuffer *pVarBuf,int size, void* pBuf),(pVarBuf,size,pBuf),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_appendbuf, \
(VariableBuffer *pVarBufDest, VariableBuffer *pVarBufToAppend), \
(pVarBufDest,pVarBufToAppend),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_print,(FILE *fp, VariableBuffer *pVarBuf),(fp, pVarBuf),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,buffer_str,(char *result, VariableBuffer *pVarBuf),(result, pVarBuf),0)
