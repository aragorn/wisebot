// ���� ifdef ����� DLL���� ���������ϴ� �۾��� ���� �� �ִ� ��ũ�θ� ����� 
// ǥ�� ����Դϴ�. �� DLL�� ��� �ִ� ������ ��� ����ٿ� ���ǵ� DHA_EXPORTS ��ȣ��
// �����ϵǸ�, ������ DLL�� ����ϴ� �ٸ� ������Ʈ������ �� ��ȣ�� ������ �� �����ϴ�.
// �̷��� �ϸ� �ҽ� ���Ͽ� �� ������ ��� �ִ� �ٸ� ��� ������Ʈ������ 
// DHA_API �Լ��� DLL���� �������� ������ ����,
// �� DLL�� �ش� ��ũ�η� ���ǵ� ��ȣ�� ���������� ������ ���ϴ�.
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