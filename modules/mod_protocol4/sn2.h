/**************************************************************
 *
 *	SYNAP NEXT2 Filter Version 2.2
 *	Copyright (C) 2001 SuperSmart Corp. Confidential.
 *	Author Email: allen@supersmart.co.kr
 *
 *	이 프로그램은 (주)수퍼스마트의 자산입니다.
 *  (주)수퍼스마트의 서면 동의없이 복제하거나
 *  부분 도용할 수 없습니다.
 *
 *	Description :
 *
 *  MS Word, PowerPoint, Excel, Hwp, PDF Filter
 *
 *
 *	Revisions:
 *
 *
 **************************************************************
 */

#ifndef SN2_H
#define SN2_H

/***************************************************************/

#define SN2_OK		0	// Successful Operation

#define SN2_ENOENT	2	// No such file or directory
#define SN2_ENOF	9	// Unknown File Format
#define SN2_EBADF	10	// Bad File Format
#define SN2_EPASS	11	// Password Exists
#define SN2_ENOMEM	12	// Not enough memory
#define SN2_EACCES	13	// Permission denied
#define SN2_ENOFLT	14	// No Format Filter
#define SN2_NOK		-1	// Unknown Error

#define SN2_ENOT	20	// No Text
#define SN2_ENOMT	21	// No More Text
#define SN2_SLAST	22	// Last Text

/***************************************************************/

#define SN2_NULL			((char *)0)

#define SN2_NO_CONTENTS		0
#define	SN2_MORE_CONTENTS	1


/***************************************************************/
/*
 * 파일포맷 정의
 */

#define SN2_FILE_UNKNOWN	0
#define SN2_FILE_DOC		1
#define SN2_FILE_PPT		2
#define SN2_FILE_XLS		3
#define SN2_FILE_OLE		4
#define SN2_FILE_HWP		5
#define SN2_FILE_HTM		6
#define SN2_FILE_RTF		7
#define SN2_FILE_TXT		8
#define SN2_FILE_MP3		9
#define SN2_FILE_ZIP		10
#define SN2_FILE_PDF		11
#define SN2_FILE_HWD		32

/***************************************************************/
/*
 * 요약정보를 담을 구조체
 */

struct t_sn2_summary {
	int 	file_type;	// 파일포맷
	char	*app_name;	// 응용프로그램
	char	*title;		// 제목
	char	*subject;	// 주제
	char	*author;	// 저자
	char	*keywords;	// 키워드
	char	*comments;	// 기타
};

typedef struct t_sn2_summary sn2_summary;

/***************************************************************/

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

char* sn2_get_program();
char* sn2_get_version();

int sn2_detect( const char  *path );
int sn2m_detect(char *data, int len);

int sn2_file_exists(const char *path);
sn2_summary * sn2_new_summary(void);
void sn2_free_summary(sn2_summary *summary);
int sn2_read_summary(const char *path, sn2_summary *summary);
int sn2m_read_summary(char *data, int len, sn2_summary *summary);

int sn2_filter(const char *path, char *buf, int len);
int sn2_doc2txt(const char *path, char *buf, int len);
int sn2_ppt2txt(const char *path, char *buf, int len);
int sn2_xls2txt(const char *path, char *buf, int len);
int sn2_hwp2txt(const char *path, char *buf, int len);
int sn2_pdf2txt(const char *path, char *buf, int len);
int sn2_htm2txt(const char *path, char *buf, int len);
int sn2_mp32txt(const char *path, char *buf, int len);
int sn2_zip2txt(const char *path, char *buf, int len);
int sn2_txt2txt(const char *path, char *buf, int len);
int sn2_rtf2txt(const char *path, char *buf, int len);
int sn2_hwd2txt(const char *path, char *buf, int len);

int sn2_filter_page(const char *path, char *buf, int len);
int sn2_pdf2txt_page(const char *path, char *buf, int len);

int sn2m_filter(char *data, int len, char *buf, int buf_len);
int sn2m_doc2txt(char *data, int len, char *buf, int buf_len);
int sn2m_ppt2txt(char *data, int len, char *buf, int buf_len);
int sn2m_xls2txt(char *data, int len, char *buf, int buf_len);
int sn2m_hwp2txt(char *data, int len, char *buf, int buf_len);
int sn2m_pdf2txt(char *data, int len, char *buf, int buf_len);
int sn2m_htm2txt(char *data, int len, char *buf, int buf_len);
int sn2m_mp32txt(char *data, int len, char *buf, int buf_len);
int sn2m_zip2txt(char *data, int len, char *buf, int buf_len);
int sn2m_txt2txt(char *data, int len, char *buf, int buf_len);
int sn2m_rtf2txt(char *data, int len, char *buf, int buf_len);
int sn2m_hwd2txt(char *data, int len, char *buf, int buf_len);

int sn2m_filter_page(char *data, int len, char *buf, int buf_len);
int sn2m_pdf2txt_page(char *data, int len, char *buf, int buf_len);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* SN2_H */
