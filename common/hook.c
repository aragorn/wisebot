/* $Id$ */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define CORE_PRIVATE 1
#include "common_core.h"
#include "log_error.h"
#include "hook.h"

const char *current_hooking_module = NULL;
int debug_module_hooks = 0;

#define MAX_HOOK_LINK_ARRAY	(20)

static void make_array_core(hook_link_array_t * res, int num, size_t size)
{
	if (num < 1)
		num = 1;

	res->elts = (void *) calloc(num, size);
	if (res->elts == NULL) {
		error("mem alloc failure for hook_link_array.");
		exit(1);
	}

	res->elt_size = size;
	res->nelts = 0;				/* No active element yet */
	res->nalloc = num;			/* but this many allocated */
}

SB_DECLARE(hook_link_array_t*) make_hook_link_array(size_t size)
{
	hook_link_array_t *res;

	res = (hook_link_array_t *) calloc(1, sizeof(hook_link_array_t));
	if (res == NULL) {
		error("mem alloc failure for hook_link_array.");
		exit(1);
	}
	make_array_core(res, MAX_HOOK_LINK_ARRAY, size);

	return res;
}

/* XXX: This must match ns_LINK_##name structure */
typedef struct
{
    void (*dummy)(void *);
    const char *szName;
    const char * const *aszPredecessors;
    const char * const *aszSuccessors;
    int nOrder;
} TSortData;

typedef struct tsort_
{
    void *pData;
    int nPredecessors;
    struct tsort_ **ppPredecessors;
    struct tsort_ *pNext;
} TSort;

static int crude_order(const void *a_,const void *b_)
{
    const TSortData *a=a_;
    const TSortData *b=b_;

    return a->nOrder-b->nOrder;
}
static TSort *prepare(TSortData *pItems,int nItems)
{
    TSort *pData=calloc(nItems,sizeof(*pData));
    int n;

    qsort(pItems,nItems,sizeof *pItems,crude_order);
    for(n=0 ; n < nItems ; ++n) {
		pData[n].nPredecessors=0;
		pData[n].ppPredecessors=calloc(nItems,sizeof(*pData[n].ppPredecessors));
		pData[n].pNext=NULL;
		pData[n].pData=&pItems[n];
    }

    for(n=0 ; n < nItems ; ++n) {
		int i,k;
		for(i=0 ; pItems[n].aszPredecessors && pItems[n].aszPredecessors[i] ; ++i)
		{
			for(k=0 ; k < nItems ; ++k)
				if(pItems[k].szName && !strcmp(pItems[k].szName,pItems[n].aszPredecessors[i])) {
					int l;

					for(l=0 ; l < pData[n].nPredecessors ; ++l)
						if(pData[n].ppPredecessors[l] == &pData[k])
							goto got_it;
					pData[n].ppPredecessors[pData[n].nPredecessors]=&pData[k];
					++pData[n].nPredecessors;
				got_it:
					break;
				}
		}
		for(i=0 ; pItems[n].aszSuccessors && pItems[n].aszSuccessors[i] ; ++i)
		{
			for(k=0 ; k < nItems ; ++k)
				if(pItems[k].szName && !strcmp(pItems[k].szName,pItems[n].aszSuccessors[i])) {
					int l;

					for(l=0 ; l < pData[k].nPredecessors ; ++l)
						if(pData[k].ppPredecessors[l] == &pData[n])
							goto got_it2;
					pData[k].ppPredecessors[pData[k].nPredecessors]=&pData[n];
					++pData[k].nPredecessors;
				got_it2:
					break;
				}
		}
    }

    return pData;
}

static TSort *tsort(TSort *pData,int nItems)
{
    int nTotal;
    TSort *pHead=NULL;
    TSort *pTail=NULL;

    for(nTotal=0 ; nTotal < nItems ; ++nTotal) {
		int n,i,k;

		for(n=0 ; ; ++n) {
			if(n == nItems) {
				error("hook[which has %s] has loop in its order",
						((TSortData*)pData->pData)->szName);
				assert(0);      /* we have a loop... */
			}
			if(!pData[n].pNext && !pData[n].nPredecessors)
				break;
		}
		if(pTail)
			pTail->pNext=&pData[n];
		else
			pHead=&pData[n];
		pTail=&pData[n];
		pTail->pNext=pTail;     /* fudge it so it looks linked */
		for(i=0 ; i < nItems ; ++i)
			for(k=0 ; pData[i].ppPredecessors[k] ; ++k)
				if(pData[i].ppPredecessors[k] == &pData[n]) {
					--pData[i].nPredecessors;
					break;
				}
    }
    pTail->pNext=NULL;  /* unfudge the tail */
    return pHead;
}

static hook_link_array_t *sort_hook(hook_link_array_t *pHooks,
							const char *szName)
{
	TSort *pSort;
	hook_link_array_t *pNew;
	int n;

	pNew=calloc(1,sizeof(hook_link_array_t));
	pSort=prepare((TSortData*)pHooks->elts,pHooks->nelts);
	pSort=tsort(pSort,pHooks->nelts);
	make_array_core(pNew,pHooks->nelts,sizeof(TSortData));
	if (debug_module_hooks)
		printf("Sorting:%s",szName);

	for (n=0; pSort; pSort=pSort->pNext,++n) {
		TSortData *pHook;
		assert(n<pHooks->nelts);
		pHook=get_last_arrayptr(pNew);
		memcpy(pHook,pSort->pData,sizeof *pHook);
		if (debug_module_hooks)
			printf(" %s",pHook->szName);
	}

	if (debug_module_hooks)
		fputc('\n',stdout);

	return pNew;
}

static hook_link_array_t *s_aHooksToSort;
typedef struct
{
	const char *szHookName;
	hook_link_array_t **paHooks;
} HookSortEntry;

#define MAX_HOOK_MODULE_NUM	30
#define TOTAL_API_LIMIT	(MAX_HOOK_LINK_ARRAY*MAX_HOOK_MODULE_NUM)


void hook_sort_register(const char *szHookName, 
						hook_link_array_t **paHooks)
{
	HookSortEntry *pEntry;

	if(!s_aHooksToSort){
		s_aHooksToSort=calloc(1,sizeof(hook_link_array_t));
		make_array_core(s_aHooksToSort,TOTAL_API_LIMIT,
						sizeof(HookSortEntry));
	}
	
	pEntry=get_last_arrayptr(s_aHooksToSort);
	pEntry->szHookName=szHookName;
	pEntry->paHooks=paHooks;
}

SB_DECLARE(void) sb_sort_hook()
{
	int n;

	if ( s_aHooksToSort == NULL ) return;

	for (n=0; n <s_aHooksToSort->nelts; ++n) {
		HookSortEntry *pEntry=&((HookSortEntry*)s_aHooksToSort->elts)[n];
		*pEntry->paHooks=sort_hook(*pEntry->paHooks,pEntry->szHookName);
	}
}

/**
 * XXX: get_last_arrayptr HAS side effect.
 *      when calling get_last_arrayptr, 
 *      you MUST assign an element to that pointer.
 */
// XXX: change function name to array_push ?? --jiwon
void* 
get_last_arrayptr(hook_link_array_t* hook_link_array)
{
	if (hook_link_array->nelts >= hook_link_array->nalloc)
		warn("hook_link_array is full.");
	else
		hook_link_array->nelts++;

	return hook_link_array->elts +
		(hook_link_array->elt_size * (hook_link_array->nelts - 1));
}

SB_DECLARE(void)
show_hook(const char *name, const char *const *aszPre,
		  const char *const *aszSucc)
{
	int first;

	printf("  Hooked %s", name);
	if (aszPre) {
		fputs(" pre(", stdout);
		first = 1;
		while (*aszPre) {
			if (!first)
				fputc(',', stdout);
			first = 0;
			fputs(*aszPre, stdout);
			++aszPre;
		}
		fputc(')', stdout);
	}
	if (aszSucc) {
		fputs(" succ(", stdout);
		first = 1;
		while (*aszSucc) {
			if (!first)
				fputc(',', stdout);
			first = 0;
			fputs(*aszSucc, stdout);
			++aszSucc;
		}
		fputc(')', stdout);
	}
	fputc('\n', stdout);
}

#if 0
void main()
{
    const char *aszAPre[]={"b","c",NULL};
    const char *aszBPost[]={"a",NULL};
    const char *aszCPost[]={"b",NULL};
    TSortData t1[]=
    {
    { NULL,"a",aszAPre,NULL },
    { NULL,"b",NULL,aszBPost },
    { NULL,"c",NULL,aszCPost }
    };
    TSort *pResult;

	printf("start\n");

    pResult=prepare(t1,3);
	printf("prepared\n");
	fflush(stdout);
    pResult=tsort(pResult,3);

	printf("test\n");
	fflush(stdout);
    for( ; pResult ; pResult=pResult->pNext)
    	printf("%s\n",((TSortData*)pResult->pData)->szName);
}
#endif
