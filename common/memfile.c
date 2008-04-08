/* $Id$ */
/* moved to common_core.h due to precompiled header 
#include <string.h> // memcpy(3),strlen(3) 
#include <errno.h>
*/
#include "common_core.h"
/*
#include "memory.h"
#include "mprintf.h"
#include "memfile.h"
*/

static unsigned int NUMALLOCOBJ = 5;
static unsigned int BLOCKSIZE = MEMFILE_LINE_LEN;



//-------------------------------------------------------------------
//     PUBLIC METHODS
//-------------------------------------------------------------------
memfile *
memfile_new(void){
	memfile *this;
	this = (memfile *)sb_malloc(sizeof(memfile));
	if(!this){
		error("sb_malloc : memfile");
		return NULL;
	}
	this->endFlag = 0; 
	this->offset = 0;
	this->mSize = 0;
	this->maxBlock = 0;
	this->numBlock = 0;
	this->bufBlock = NULL;
	return this;
}
//----------------------------------------------------------------
void
memfile_reset(memfile *this) {
	int idx;
	this->mSize = 0;
	this->endFlag = 0;
	this->offset = 0;

	if (this->bufBlock != NULL){
		for(idx = 0; idx < this->numBlock; idx++) {
			sb_free(this->bufBlock[idx]);
			this->bufBlock[idx] = NULL;
		}
		sb_free(this->bufBlock);
		this->bufBlock = NULL;
	}
	this->numBlock = 0;
	this->maxBlock = 0;
}
//----------------------------------------------------------------
void
memfile_free(memfile *this) {
	memfile_reset(this);
	sb_free(this);
}
//-------------------------------------------------------------------
inline unsigned long 
memfile_getSize(memfile *this) {
	return this->mSize;
}
//-------------------------------------------------------------------
int
memfile_setSize(memfile *this, unsigned long size){
	int wantedNumOfBlk = (size / BLOCKSIZE) + 1;
	if(wantedNumOfBlk > this->numBlock) {
		if(wantedNumOfBlk > this->maxBlock) {
			int i;
			char **tmp;
			while(wantedNumOfBlk > this->maxBlock) {
				this->maxBlock += NUMALLOCOBJ;
			}
			
			tmp = (char **)sb_calloc(this->maxBlock, sizeof(char *));
			if(tmp == NULL) {
				error("Failed To allocate new memory blocks lump");
				return -1;
			}
			if(this->bufBlock){
				memcpy(tmp, this->bufBlock, sizeof(char *) * this->numBlock);
				sb_free(this->bufBlock);
			}
			this->bufBlock = tmp;

			for(i = this->numBlock; i < this->maxBlock; i++) {
				this->bufBlock[i] = NULL;
			}
		}

		while(wantedNumOfBlk > this->numBlock) {
			this->bufBlock[this->numBlock] = (char *) sb_malloc(BLOCKSIZE);
			if(this->bufBlock[this->numBlock] == NULL) {
				error("Failed To allocate a new memory block");
				return -1;
			}
			this->numBlock++;
		}
	} else {
		int idx;
		if ( wantedNumOfBlk < this->numBlock )
			DEBUG("trying to delete last [%ld]byte", memfile_getSize(this)-size);
		for(idx = wantedNumOfBlk; idx < this->numBlock; idx++) {
			sb_free(this->bufBlock[idx]);
			this->bufBlock[idx] = NULL;
		}
		this->numBlock = wantedNumOfBlk;
	}

	this->mSize = size;
	return this->mSize;
}
//-------------------------------------------------------------------
int memfile_shift(memfile *this, unsigned long size){
	int i;
	unsigned long move_size = this->mSize - size;

	for ( i = 0; i < move_size; i++ ) {
		*(this->bufBlock[i/BLOCKSIZE]+i%BLOCKSIZE) =
			*(this->bufBlock[(size+i)/BLOCKSIZE]+(size+i)%BLOCKSIZE);
	}
	this->mSize -= size;
	this->offset -= size;
	return size;
}
//-------------------------------------------------------------------
inline unsigned long
memfile_getOffset(memfile *this){
	return this->offset;
}
//-------------------------------------------------------------------
inline void
memfile_setOffset(memfile *this, unsigned long offset){
	this->offset = offset;
}
//-------------------------------------------------------------------
inline unsigned char
memfile_isEndOfAppend(memfile *this){
	return this->endFlag;
}
//-------------------------------------------------------------------
inline void
memfile_setEndOfAppend(memfile *this){
	this->endFlag = 1;
}
//-------------------------------------------------------------------
int
memfile_read(memfile *this, char *buf, int size) {
	int blkIdx, blkOffset;
	int dSize;
	
	if(buf == NULL) {
		error("buffer to read in from this memfile is NULL");
		return -1;
	}
	
	blkIdx = this->offset / BLOCKSIZE;
	blkOffset = this->offset % BLOCKSIZE;

	if( this->offset+size > this->mSize) { 
		DEBUG("there are not as many characters as requested."
			  " offset=[%ld] reqSize=[%d] totSize=[%ld]"
			   , this->offset, size, this->mSize);
		size = this->mSize - this->offset;
	}
	
	for( dSize =0; dSize < size; dSize++) {
		buf[dSize] = *(this->bufBlock[blkIdx]+blkOffset);
		blkOffset++;
		if(blkOffset == BLOCKSIZE) {
			blkOffset = 0;
			blkIdx++;
		}
	}
	this->offset += dSize;
	return dSize;

}
//-------------------------------------------------------------------
int
memfile_read2memfile (memfile *this, memfile *mFile, int size){
	char	buf[MEMFILE_LINE_LEN];
	int		readLen = 0, len, sizeToRead;
	unsigned long 	offset = memfile_getOffset(this);

	while (readLen < size) {
	 	sizeToRead = ( MEMFILE_LINE_LEN > (size-readLen) ) ? 
						size-readLen : MEMFILE_LINE_LEN;
		memfile_setOffset(this, offset);
		len = memfile_read(this, buf, sizeToRead);
		len = memfile_append(mFile, buf, len);
		if( len < 0 ){
			error("failed to append to a memfile : mFile ");
			memfile_setOffset(this, offset);
			return -1;
		}			
		readLen += len;
		offset += len;

		if(len < MEMFILE_LINE_LEN ) { // no more data
			break;
		}
	}

	return readLen;
}
//-------------------------------------------------------------------
int
memfile_append(memfile *this, const char *buf, int size){
	int neededBlk, blkIdx, blkOffset;
	int dSize;

	if(size == 0 )	return 0;
	if(buf == NULL || size < 0 ) {
		WARN("buffer requested to add is null[%p] or size[%d] is negative value"
				, buf, size);
		return -1;
	}

	neededBlk = ((this->offset + size) / BLOCKSIZE )  + 1;
	if(neededBlk > this->numBlock) {
		if(neededBlk > this->maxBlock) {
			int i;
			char **tmp;
			while(neededBlk > this->maxBlock) {
				this->maxBlock += NUMALLOCOBJ;
			}
			tmp = (char **)sb_calloc(this->maxBlock, sizeof(char *));
			if(tmp == NULL) {
				error("Failed To allocate new memory blocks lump");
				return -1;
			}
			if ( this->bufBlock ){
				memcpy(tmp, this->bufBlock, sizeof(char *) * this->numBlock);
				sb_free(this->bufBlock);
			}
			this->bufBlock = tmp;
			for(i = this->numBlock; i < this->maxBlock; i++) {
				this->bufBlock[i] = NULL;
			}
		}

		while(neededBlk > this->numBlock) {
			this->bufBlock[this->numBlock] = (char *) sb_malloc(BLOCKSIZE);
			if(this->bufBlock[this->numBlock] == NULL) {
				error("Failed To allocate a new memory block");
				return -1;
			}
			this->numBlock++;
		}
	}

	blkIdx = this->offset / BLOCKSIZE;
	blkOffset = this->offset % BLOCKSIZE;
	
	for( dSize =0; dSize < size; dSize++) {
		*(this->bufBlock[blkIdx]+blkOffset) = buf[dSize];
		blkOffset++;
		if(blkOffset == BLOCKSIZE ) {
			blkOffset = 0;
			blkIdx++;
		}
	}
	this->offset += dSize;
	if ( this->offset > this->mSize ) {
		this->mSize = this->offset;
	}
	return dSize;
}

//-------------------------------------------------------------------
int
memfile_appendF(memfile *this, const char *format, ... ){
	int appendedSize = 0;
	char *s;
	va_list ap;
	va_start(ap, format);
	s = curl_mvaprintf(format, ap);
	va_end(ap);

	if( s == NULL ){
		error("curl_mvaprintf failed :  [%s]",format);
		return -1;
	}
	appendedSize = memfile_append(this, s, strlen(s));
	sb_free(s);
	return appendedSize;
}
		

//-------------------------------------------------------------------
// not in use .. 
//
// thisLine번째 문자부터, "\n" 문자를 만날 때까지의 길이를 잰다.
// 즉 ,thisLine 번째 문자가 들어있는 줄의,
// thisLine번째 문자부터 센 길이를 리턴하는 것이다.
// return length of this line from 'thisLine' including '\n'
void
memfile_getLineLen(memfile *this, int thisLine, int *lineLenGot) {

	int blkIdx = thisLine / BLOCKSIZE;
	int blkOffset = thisLine % BLOCKSIZE;
	int offset;
	for( offset = thisLine; offset < this->mSize; offset++) {
		if( *(this->bufBlock[blkIdx]+blkOffset) == '\n') {
			break;
		}
		blkOffset++;
		if(blkOffset == BLOCKSIZE) {
			blkOffset = 0;
			blkIdx++;
		}
	}

	if(offset < this->mSize) {
		*lineLenGot = (offset+1) - thisLine;
	} else {
		if(offset == this->mSize) {
			*lineLenGot = this->mSize - thisLine;
		} else {
			*lineLenGot = 0;
		}
	}
}
//-------------------------------------------------------------------
void
memfile_print(memfile *this) {
	int blkIdx,blkOffset,dSize;
	printf("----------- BEGIN : memfile --------\n");
	printf("size[%ld], offset [%ld] endFlag[%s], maxBlock[%d], numBlock[%d]\n",
		this->mSize, this->offset, (this->endFlag) ? "true" : "false", 
		this->maxBlock, this->numBlock);
	printf("Contents : \n");

	blkIdx = 0;
	blkOffset = 0;

	for(dSize =0; dSize < this->mSize; dSize++) {
		putchar(*(this->bufBlock[blkIdx]+blkOffset));
		blkOffset++;
		if(blkOffset == BLOCKSIZE) {
			blkOffset = 0;
			blkIdx++;
		}
	}
	putchar('\n');
	printf("----------- END : memfile --------\n");
	fflush(stdout);
}
//-------------------------------------------------------------------
//------------------------------------------------------------------------------
int
memfile_readFromFile(memfile *this, char *path){
    FILE    *fp;
    char 	buf[MEMFILE_LINE_LEN];
	int		readSize;

    if((fp = sb_fopen(path,"r")) == NULL) {
		error("file open to read Failed %s - %s ", path, strerror(errno));
        return -1;
    }

    while(!feof(fp)) {
		readSize = fread(buf, sizeof(char), MEMFILE_LINE_LEN, fp);
	   	if( readSize <= 0 ){
/*        if(fgets(buf,LINE_LEN,fp) == NULL){*/
            break;
        }
        if(memfile_append(this, buf, readSize) < 0){
			error("memfile_append failed");
            fclose(fp);
            return -1;
        }
    }
	fclose(fp);
    memfile_setEndOfAppend(this);

    return 1;
}

//------------------------------------------------------------------------------
int
memfile_writeToFile(memfile *this, char *path, unsigned long size){
    char buf[MEMFILE_LINE_LEN];
    FILE    *fp;
    int     writtenSize = 0, tempReadSize=0;
	unsigned long initOffset = memfile_getOffset(this);
	

    if((fp = sb_fopen(path,"w")) == NULL) {
        error("file open to write Failed %s - %s", path, strerror(errno));
        return -1;
    }

    while(writtenSize < size){
		tempReadSize = ( size - writtenSize > MEMFILE_LINE_LEN ) ?
							MEMFILE_LINE_LEN : size - writtenSize;
		
		memfile_setOffset(this, initOffset + writtenSize);
       	tempReadSize = memfile_read(this, buf, tempReadSize);
		tempReadSize = fwrite(buf, sizeof(char), tempReadSize, fp);
		if (tempReadSize <= 0 ){
			memfile_setOffset(this, initOffset);
			return -1;
		}
		writtenSize += tempReadSize;
    }
    fclose(fp);
    return writtenSize;
}
