/* $Id$ */
/** 
 *	@memo ds.h
 *	@name ds.h
 *	@author gramo & woosong
 *	@modifier jiwon
 *	@modifier nominam
 *	@long:  DID Server module header
 *	@date: 2000.01.15
 */

#ifndef _DS_H_
#define _DS_H_

#include "softbot.h"
/*
typedef int int16_t;
typedef unsigned char int8_t;
*/

///Did의 상태를 표시한다.
struct tagDidState {
	///Document의 유무
	int8_t bDoc : 1;
	///Anchor text의 유무
	int8_t bAnc : 1;
};
typedef struct tagDidState CDidState;

///
#define SYS_DID_KEYSIZE (6)

///Did Slot
typedef struct {
	///MD5 키 값
	char szKey[SYS_DID_KEYSIZE];
	///Did 값
	DocId docId;
	///State값
	CDidState byState;
} did_slot_t;

#define MAX_LOAD_BLOCK 	(100000)
///
#define MAX_DID_SL		(1024)
///
#define MAX_DID_BK		(65536*2)
///
#define MAX_PC			(1)
///
#define MAX_URL_LENGTH  (256)
///Did Bucket
struct tagDidBK {
	///Slot
	did_slot_t	aDidSL[MAX_DID_SL];
	///Slot count
	int16_t	nSlotCnt;
	///Bucket prefix bit : used for partitioning check
	int8_t	byPrefixBit;
};
typedef struct tagDidBK CDidBK;

///Did Send Packet structure
struct tagDidSendPacket {
	int8_t byOrder;
	/// state
	int8_t byState;
	/// slot
	did_slot_t szDidSL;
};
typedef struct tagDidSendPacket CDidSendPacket;

///Did Receive Packet structure
struct tagDidReceivePacket {
	int8_t byOrder;
	/// Operation
	int8_t byOperation;
	int16_t nLen;
	/// state;
	CDidState szState;
	/// URL
	char szURL[MAX_URL_LENGTH];
};
typedef struct tagDidReceivePacket CDidReceivePacket;

/// Use this when DIDS send  Packet
#define DID_STATE_NEW	(1)
#define DID_STATE_OLD	(2)
#define NOTFOUND		(3)

/// Use this when DIDS receive Packet
#define DID_OPERATION_UPDATE_ONLY	(1)
#define DID_OPERATION_FIND_AND_NEW	(2)
#define DID_OPERATION_FIND_ONLY		(3)
	
///Did Bucket Table(for dynamic hashing)
struct tagDidBKTable {
	///Hash prefix bit
	int8_t	byPrefixBit;
	///Bucket pointer
	CDidBK	*pDidBK[MAX_DID_BK];
};
typedef struct tagDidBKTable CDidBKTable;

CDidBKTable *gDidBKTable;
//int16_t gDSSemID;

#endif
