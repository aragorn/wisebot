#include "common_core.h"
		
#ifndef VRM_H
#define VRM_H

typedef struct vrm_t vrm_t;

SB_DECLARE_HOOK(int, vrm_open, (char path[], vrm_t **vrm))
SB_DECLARE_HOOK(int, vrm_close, (vrm_t *vrm))
SB_DECLARE_HOOK(int, vrm_read, (vrm_t *vrm, int key, void** data, int* size))
SB_DECLARE_HOOK(int, vrm_add, (vrm_t *vrm, int key, void* data, int size))
SB_DECLARE_HOOK(int, vrm_status, (vrm_t *vrm))

#endif
