/* $Id$ */
#ifndef	_MEMFILE_H_
#define	_MEMFILE_H_

typedef struct {
	unsigned char	endFlag;	//0 : not End , 1 : IsEnd
	unsigned long	offset;		//begins at 0;
	unsigned long	mSize;		
	int	maxBlock;
	int numBlock;	
	char 	**bufBlock;
} memfile;

#define MEMFILE_LINE_LEN 	(512)

memfile *
memfile_new(void);

void
memfile_reset(memfile *pMF);

void
memfile_free(memfile *pMF);
	


unsigned long
memfile_getSize(memfile *pMF);

//size 이후의 character들이 무시된다. 
//return : size After resizing.
int
memfile_setSize(memfile *pMF, unsigned long size);

//size 만큼 앞에서부터 shift out 한다.
int memfile_shift(memfile *this, unsigned long size);

unsigned long
memfile_getOffset(memfile *pMF);

void
memfile_setOffset(memfile *pMF, unsigned long offset);

unsigned char
memfile_isEndOfAppend(memfile *pMF);

void   
memfile_setEndOfAppend(memfile *pMF);

// memfile의 offset 부터 size 만큼의 문자를
// buf에 읽어들인다. 읽어들여진 양을 return한다. 
int    
memfile_read(memfile *pMF, char *buf, int size );

// memfile의 offset 부터 size만큼의 문자를 
// mFile에 읽어서 넣는다. 읽어들인 양을 return한다.
int    
memfile_read2memfile(memfile *pMF, memfile *mFile, int size);


// memfile에  buf가 가리키는 곳부터 size만큼의 양을 append한다.
// append 된 사이즈를 반환한다.
int    
memfile_append(memfile *pMF, char *buf, int size );

// memfile에 format에 해당하는 내용을 프린트해넣는다.
// append 된 사이즈를 반환한다.
int
memfile_appendF(memfile *pMF, const char *format, ... );



// memfile에 주어진 파일을 읽어들인다. 
int
memfile_readFromFile(memfile *pMF, char *path);

// MEMFILE을 주어진 file에 쓴다.
// offset부터 size만큼
int
memfile_writeToFile(memfile *pMF, char *path, unsigned long size);

void  
memfile_print(memfile *pMF);

#endif // _MEMFILE_H
