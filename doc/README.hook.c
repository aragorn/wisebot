/* $Id$ */

in register_hooks , mod_status.c 
	ap_hook_handler(status_handler, NULL, NULL, APR_HOOK_MIDDLE);


in http_config.h
  AP_DECLARE_HOOK(int, handler, (request_rec *r))

in ap_config.h
  #define AP_DECLARE_HOOK(ret,name,args) \
      APR_DECLARE_EXTERNAL_HOOK(ap,AP,ret,name,args)

	macro 확장을 한 후..
	  APR_DECLARE_EXTERNAL_HOOK(ap,AP,int,handler,(request_rec *r))

in apr_hooks.h
#define APR_DECLARE_EXTERNAL_HOOK(ns,link,ret,name,args) \
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

	// XXX macro 확장을 한 후..

typedef int ap_HOOK_handler_t (request_rec *r);
AP_DECLARE(void) ap_hook_handler(ap_HOOK_handler_t *pf,
								const char *const *aszPre,
								const char *const *aszSucc,int nOrder);
AP_DECLARE(int) ap_run_handler (request_rec *r);
typedef struct ap_LINK_handler_t
	{
		ap_HOOK_handler_t *pFunc;
		const char *szName;
		const char *const *aszPredecessors;
		const char *const *aszSuccessors;
		int nOrder;
	} ap_LINK_handler_t;

참고: #define AP_DECLARE(type)            type 

in server/config.c 
  AP_IMPLEMENT_HOOK_RUN_FIRST(int,handler,(request_rec *r),(r),DECLINED)

  in ap_config.h
    #define AP_IMPLEMENT_HOOK_RUN_FIRST(ret,name,args_decl,args_use,decline) \
          APR_IMPLEMENT_EXTERNAL_HOOK_RUN_FIRST(ap,AP,ret,name,args_decl, \
	                                                     args_use,decline)
	// XXX macro 확장 후 
APR_IMPLEMENT_EXTERNAL_HOOK_RUN_FIRST(ap,AP,int,handler,(request_rec *r), (r),
																		DECLINED)

    in srclib/apr-util/include/apr_hooks.h
#define APR_IMPLEMENT_EXTERNAL_HOOK_RUN_FIRST(ns,link,ret,name,args_decl,args_use,decline) \
APR_IMPLEMENT_EXTERNAL_HOOK_BASE(ns,link,name) \
link##_DECLARE(ret) ns##_run_##name args_decl \
    { \
    ns##_LINK_##name##_t *pHook; \
    int n; \
    ret rv; \
\
    if(!_hooks.link_##name) \
    return decline; \
\
    pHook=(ns##_LINK_##name##_t *)_hooks.link_##name->elts; \
    for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
    { \
    rv=pHook[n].pFunc args_use; \
\
    if(rv != decline) \
        return rv; \
    } \
    return decline; \
    }

	// XXX macro 확장 후..
APR_IMPLEMENT_EXTERNAL_HOOK_BASE(ap,AP,handler)
AP_DECLARE(int) ap_run_handler (request_rec *r)
	{
		ap_LINK_handler_t *pHook;
		int n;
		int rv;

		if (!_hooks.link_handler)
			return DECLINED;

		pHook=(ap_LINK_handler_t *)_hooks.link_handler->elts;
		for(n=0; n< _hooks.link_handler->nelts; ++n)
		{
			rv=pHook[n].pFunc (r);

			if(rv != DECLINED)
				return rv;
		}
		return DECLINED;
	}

여기서 APR_IMPLEMENT_EXTERNAL_HOOK_BASE에 대해..
in apr_hooks.h
#define APR_IMPLEMENT_EXTERNAL_HOOK_BASE(ns,link,name) \
link##_DECLARE(void) ns##_hook_##name(ns##_HOOK_##name##_t *pf,const char * const *aszPre, \
                                      const char * const *aszSucc,int nOrder) \
    { \
    ns##_LINK_##name##_t *pHook; \
    if(!_hooks.link_##name) \
    { \
    _hooks.link_##name=apr_array_make(apr_global_hook_pool,1,sizeof(ns##_LINK_##name##_t)); \
    apr_hook_sort_register(#name,&_hooks.link_##name); \
    } \
    pHook=apr_array_push(_hooks.link_##name); \
    pHook->pFunc=pf; \
    pHook->aszPredecessors=aszPre; \
    pHook->aszSuccessors=aszSucc; \
    pHook->nOrder=nOrder; \
    pHook->szName=apr_current_hooking_module; \
    if(apr_debug_module_hooks) \
    apr_show_hook(#name,aszPre,aszSucc); \
    }
AP_DECLARE(void) ap_hook_handler(ap_HOOK_handler_t *pf, const char * const *aszPre,\
									const char * const *aszSucc, int nOrder)
	{
		ap_LINK_handler_t *pHook;
		if(!_hooks.link_handler)
		{
			_hooks.link_handler=apr_array_make(apr_global_hook_pool,1,
														sizeof(ap_LINK_handler_t));
			apr_hook_sort_register("handler",&_hooks.link_handler);
		}
		pHook=apr_array_push(_hooks.link_handler);
		pHook->pFunc=pf;
		pHook->aszPredecessors=aszPre;
		pHook->aszSuccessors=aszSucc;
		pHook->nOrder=nOrder;
		pHook->szName=apr_current_hooking_module;
		if(apr_debug_module_hooks)
			apr_show_hook("handler",aszPre,aszSucc);
	}

AP_DECLARE(int) ap_run_handler (request_rec *r)
	{
		ap_LINK_handler_t *pHook;
		int n;
		int rv;

		if (!_hooks.link_handler)
			return DECLINED;

		pHook=(ap_LINK_handler_t *)_hooks.link_handler->elts;
		for(n=0; n< _hooks.link_handler->nelts; ++n)
		{
			rv=pHook[n].pFunc (r);

			if(rv != DECLINED)
				return rv;
		}
		return DECLINED;
	}


_hooks.link_handler 가 실제로 변수로 만들어 지는 부분
#define APR_HOOK_STRUCT(members) \
static struct { members } _hooks;

#define APR_HOOK_LINK(name) \
    apr_array_header_t *link_##name;

