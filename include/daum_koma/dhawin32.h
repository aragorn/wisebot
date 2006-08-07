// 다음 ifdef 블록은 DLL에서 내보내기하는 작업을 쉽게 해 주는 매크로를 만드는 
// 표준 방식입니다. 이 DLL에 들어 있는 파일은 모두 명령줄에 정의된 DHA_EXPORTS 기호로
// 컴파일되며, 동일한 DLL을 사용하는 다른 프로젝트에서는 이 기호를 정의할 수 없습니다.
// 이렇게 하면 소스 파일에 이 파일이 들어 있는 다른 모든 프로젝트에서는 
// DHA_API 함수를 DLL에서 가져오는 것으로 보고,
// 이 DLL은 해당 매크로로 정의된 기호가 내보내지는 것으로 봅니다.
#ifdef DHA_EXPORTS
#define DHA_API __declspec(dllexport)
#else
#define DHA_API __declspec(dllimport)
#endif


/*
 * initialize dha module
 *
 * @param path - the path to dictionaries
 * @return - pointer to object
 */
extern DHA_API void *dha_initialize(const char *szDicPath,const char *szConfPath);

/*
 * finalize dha module
 *
 * @param this - the pointer of object
 */
DHA_API void dha_finalize(void *dha);

/*
 * analyze hangul string & return the list of morpheme
 *
 * @param this - the pointer of analyzer object
 * @param option - option -- obsolete
 * @param str - input string
 * @param bufsize - length of _buf_
 * @param buf - buffer for output
 *
 * @return if success, 1, otherwise -1
 */
DHA_API int dha_analyze(void *dha, const char *option, char *string, int bufsize, char *buf);