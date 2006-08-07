#ifndef _HANCODE_H_
#define _HANCODE_H_

#include "hantype.h"

// 한글 테이블의 세로행의 수(Hancode.h)
#define HMAX 11172

extern hancode HANCODE[HMAX][8];
extern hancode WANSUNG[65536];
extern hancode JOHAB[65536];

#endif
