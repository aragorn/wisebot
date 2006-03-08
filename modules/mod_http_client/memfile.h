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

//size ������ character���� ���õȴ�. 
//return : size After resizing.
int
memfile_setSize(memfile *pMF, unsigned long size);

//size ��ŭ �տ������� shift out �Ѵ�.
int memfile_shift(memfile *this, unsigned long size);

unsigned long
memfile_getOffset(memfile *pMF);

void
memfile_setOffset(memfile *pMF, unsigned long offset);

unsigned char
memfile_isEndOfAppend(memfile *pMF);

void   
memfile_setEndOfAppend(memfile *pMF);

// memfile�� offset ���� size ��ŭ�� ���ڸ�
// buf�� �о���δ�. �о�鿩�� ���� return�Ѵ�. 
int    
memfile_read(memfile *pMF, char *buf, int size );

// memfile�� offset ���� size��ŭ�� ���ڸ� 
// mFile�� �о �ִ´�. �о���� ���� return�Ѵ�.
int    
memfile_read2memfile(memfile *pMF, memfile *mFile, int size);


// memfile��  buf�� ����Ű�� ������ size��ŭ�� ���� append�Ѵ�.
// append �� ����� ��ȯ�Ѵ�.
int    
memfile_append(memfile *pMF, char *buf, int size );

// memfile�� format�� �ش��ϴ� ������ ����Ʈ�سִ´�.
// append �� ����� ��ȯ�Ѵ�.
int
memfile_appendF(memfile *pMF, const char *format, ... );



// memfile�� �־��� ������ �о���δ�. 
int
memfile_readFromFile(memfile *pMF, char *path);

// MEMFILE�� �־��� file�� ����.
// offset���� size��ŭ
int
memfile_writeToFile(memfile *pMF, char *path, unsigned long size);

void  
memfile_print(memfile *pMF);

#endif // _MEMFILE_H
