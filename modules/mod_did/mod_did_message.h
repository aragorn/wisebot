/* $Id$ */
#ifndef _DI_MESG_H_
#define _DI_MESG_H_

#include "softbot.h"

// Structures used as message to/from did server

// values of nMethod
#define DITP_GET_NEW_ID (1)
#define DITP_GET_ID (2)
#define DITP_GET_NUM_DOC (3)

// definitions related to protocol
#define PROTOCOL_ID_SIZE (5)
#define MAX_DID_KEY_LEN (256)

/**
 *	if following structures get modified,
 *	docId.c/didServer.c file must examined
 *	especially, structure initiallize part
 */
typedef struct tagClientRequest{
    char aKey[MAX_DID_KEY_LEN];
	int nKeyLen;
    char nMethod;
} ClientRequest;
typedef struct tagClientMsg{
    char aProtocolId[PROTOCOL_ID_SIZE];	// must be "DITP"
    char majorVersion;
    char minorVersion;
/**
 * following check must be same to check of ServerMsg
 * returned as answer to this ClientMsg
 */
	int32_t session;
    ClientRequest mesg;
} ClientMsg;
    
typedef struct tagServerMsg{
    int status;		// can have DI_NOT_REGISTERED, DI_ID_OVERFLOW, refer docId.h
    char nMethod;	// can have DITP_GET_ID, .. see above of this file
    DocId result;	// can have DocId, numOfDoc
	DocId olddid;	// old registered docid for requested key
///	char check;		// must be same to ClientMsg->check. see above
	int32_t session;
} ServerMsg;

/* when sending or receving udp packet, waits 2 secs */
#define MSG_TIMEOUT (2)	
/* when error occurs while communicating DS server, only try up to 20 time */
#define MAX_TRY_CNT (20)

#define PORT_ID_DID	'D'

#endif
