/* $Id$ */
#ifndef HOOK_H
#define HOOK_H

#include <stdlib.h>
#include "softbot.h" /* SB_DECLARE_HOOK */

extern SB_DECLARE_DATA const char *current_hooking_module;
extern SB_DECLARE_DATA int debug_module_hooks;

// meta module이 export하는 api hook 저장구조.
// member에는 항상 HOOK_LINK(...) 들이 들어간다.
#define HOOK_STRUCT(members) \
	static struct { members } _hooks;
	
// 개별적인 api hook
#define HOOK_LINK(name) \
	hook_link_array_t	*link_##name;

typedef struct {
    /** The amount of memory allocated for each element of the array */
    int elt_size;
    /** The number of active elements in the array */
    int nelts;
    /** The number of elements allocated in the array */
    int nalloc;
    /** The elements in the array */
    void *elts;
} hook_link_array_t;

// name space가 SB인 특별한 경우의 DECLARE_HOOK
#define SB_DECLARE_HOOK(ret,name,args) \
	DECLARE_HOOK(sb,SB,ret,name,args)

/**
 * Declare a hook function
 * @param ns namespace
 * @param link macro의 namespace
 * @param ret The return type of the hook
 * @param name The hook's name (as a literal)
 * @param args The arguments the hook function takes, in brackets.
 */
#define DECLARE_HOOK(ns,link,ret,name,args) \
typedef ret ns##_HOOK_##name##_t args; \
link##_DECLARE(void) ns##_hook_##name(ns##_HOOK_##name##_t *pf, \
					  const char * const *aszPre, \
					  const char * const *aszSucc, int nOrder); \
link##_DECLARE(ret) ns##_run_##name args; \
typedef struct ns##_LINK_##name##_t \
{ \
	ns##_HOOK_##name##_t *pFunc; \
	const char *szName; \
	const char * const *aszPredecessors; \
	const char * const *aszSuccessors; \
	int nOrder; \
} ns##_LINK_##name##_t;
/* 1. api의 function type을 typedef 한다.
 * 2. ns_hook_*() 을 선언한다. (module에서 metamodule에 api를 등록시 쓰임)
 * 3. ns_run_*() 을 선언한다. (meta module에서 api를 실행할 때 쓰임)
 * 4. hook_link_array_t array의 element의 type을 typedef한다.
 **/

// name space가 SB인 특별한 경우의 IMPLEMENT_HOOK_RUN_FIRST
#define SB_IMPLEMENT_HOOK_RUN_FIRST(ret,name,args_decl,args_use,decline) \
	IMPLEMENT_HOOK_RUN_FIRST(sb,SB,ret,name,args_decl,args_use,decline)

#define SB_IMPLEMENT_HOOK_RUN_ALL(ret,name,args_decl,args_use,ok,decline) \
	IMPLEMENT_HOOK_RUN_ALL(sb,SB,ret,name,args_decl,args_use,ok,decline)

#define SB_IMPLEMENT_HOOK_RUN_VOID_ALL(name,args_decl,args_use) \
	IMPLEMENT_HOOK_RUN_VOID_ALL(sb,SB,name,args_decl,args_use)

// run function returns return value of hooked function
/*#define SB_IMPLEMENT_HOOK_RUN_ONCE(ret,name,args_decl,args_use) \
	IMPLEMENT_HOOK_RUN_ONCE(sb,SB,ret,name,args_decl,args_use)*/

/**
 * Implement a hook that runs until the first function that returns
 * something other than decline. If all functions return decline, the
 * hook runner returns decline. The implementation is called
 * ns_run_<i>name</i>.
 *
 * @param ns the namespace
 * @param link the namespace of macro
 * @param ret The return type of the hook (and the hook runner)
 * @param name The name of the hook
 * @param args_decl The declaration of the arguments for the hook, for example
 * "(int x,void *y)"
 * @param args_use The arguments for the hook as used in a call, for example
 * "(x,y)"
 * @param decline The "decline" return value
 * @return decline or an error.
 */
#define IMPLEMENT_HOOK_RUN_FIRST(ns,link,ret,name,args_decl,args_use,decline) \
IMPLEMENT_HOOK_BASE(ns,link,name) \
link##_DECLARE(ret) ns##_run_##name args_decl \
{ \
	ns##_LINK_##name##_t *pHook; \
	int n; \
	ret rv; \
\
	if(!_hooks.link_##name){ \
		warn("No function hooked"); \
		return decline; \
	} \
\
	pHook=(ns##_LINK_##name##_t *)_hooks.link_##name->elts; \
	for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	{ \
		rv=pHook[n].pFunc args_use; \
\
		if(rv != decline) \
			return rv; \
	} \
	warn("No function run(all function hooked returned decline)"); \
	return decline; \
}

#define IMPLEMENT_HOOK_RUN_ALL(ns,link,ret,name,args_decl,args_use,ok,decline) \
IMPLEMENT_HOOK_BASE(ns,link,name) \
link##_DECLARE(ret) ns##_run_##name args_decl \
{ \
	ns##_LINK_##name##_t *pHook; \
	int n; \
	ret rv; \
\
	if(!_hooks.link_##name) {\
		warn("No function hooked"); \
		return ok; \
	} \
\
	pHook=(ns##_LINK_##name##_t *)_hooks.link_##name->elts; \
	for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	{ \
		rv=pHook[n].pFunc args_use; \
\
		if(rv != ok && rv != decline) \
			return rv; \
	} \
	return ok; \
}

#define IMPLEMENT_HOOK_RUN_VOID_ALL(ns,link,name,args_decl,args_use) \
IMPLEMENT_HOOK_BASE(ns,link,name) \
link##_DECLARE(void) ns##_run_##name args_decl \
{ \
	ns##_LINK_##name##_t *pHook; \
	int n; \
\
	if(!_hooks.link_##name) {\
		warn("No function hooked"); \
		return ; \
	} \
\
	pHook=(ns##_LINK_##name##_t *)_hooks.link_##name->elts; \
	for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	{ \
		pHook[n].pFunc args_use; \
\
	} \
	return ; \
}
#if 0
#define IMPLEMENT_HOOK_RUN_ONCE(sb,SB,ret,name,args_decl,args_use) \
IMPLEMENT_HOOK_BASE_ONCE(ns,link,name) \
link##_DECLARE(ret) ns##_run_##name args_decl \
{ \
	ns##_LINK_##name##_t *pHook; \
	int n; \
	ret rv; \
\
	if(!_hooks.link_##name) \
		return ; \
\
	pHook=(ns##_LINK_##name##_t *)_hooks.link_##name->elts; \
	for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	{ \
		rv=pHook[n].pFunc args_use; \
\
	} \
	return rv; \
}
#endif

// TODO: gotta implement hook_sort_register() and sort_hooks().
// see srclib/apr-util/include/apr_hooks.h and
// srclib/apr-util/hooks/apr_hooks.c of apache 2.0.xx.
#define IMPLEMENT_HOOK_BASE(ns,link,name) \
link##_DECLARE(void) ns##_hook_##name(ns##_HOOK_##name##_t *pf,const char * const *aszPre, \
                                   const char * const *aszSucc,int nOrder) \
{ \
	ns##_LINK_##name##_t *pHook; \
	if(!_hooks.link_##name) \
	{ \
		_hooks.link_##name=make_hook_link_array(sizeof(ns##_LINK_##name##_t)); \
		hook_sort_register(#name,&_hooks.link_##name); \
	} \
	pHook=get_last_arrayptr(_hooks.link_##name); \
	pHook->pFunc=pf; \
	pHook->aszPredecessors=aszPre; \
	pHook->aszSuccessors=aszSucc; \
	pHook->nOrder=nOrder; \
	pHook->szName=current_hooking_module; \
	if(debug_module_hooks) \
		show_hook(#name,aszPre,aszSucc); \
}

#define HOOK_REALLY_FIRST 0
#define HOOK_FIRST	10
#define HOOK_MIDDLE	20
#define HOOK_LAST	30
#define HOOK_REALLY_LAST 40
SB_DECLARE(hook_link_array_t*) make_hook_link_array(size_t size);
SB_DECLARE(void*) get_last_arrayptr(hook_link_array_t* hook_link_array);
SB_DECLARE(void) show_hook(const char *name, const char * const *aszPre, const char * const *aszSucc);

SB_DECLARE(void) sb_sort_hook();
SB_DECLARE(void) hook_sort_register(const char *szHookName, 
						hook_link_array_t **aHooks);
#endif // _HOOK_H_
