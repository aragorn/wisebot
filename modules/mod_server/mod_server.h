/* $Id$ */
#ifndef _MOD_METADUMMY_H_
#define _MOD_METADUMMY_H_

#include "hook.h"

SB_DECLARE_HOOK(int,dummy_api1,(int arg1))
SB_DECLARE_HOOK(int,dummy_api2,(char *str))
//SB_DECLARE_HOOK(void,noexist,(void))
#endif
