/* $Id$
 *	modulized by nominam, 2001.05.
 */
#include "softbot.h"
#include "mod_api/did_client.h"

#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "mod_api/udp.h"
#include "mod_did_message.h"


//	functions, vars needed for getId, getNewId
static int requestServer();	

static char mIP[SHORT_STRING_SIZE] = "127.0.0.1";
static char mPort[SHORT_STRING_SIZE] = "8504";
static int mSockfd;
static char sock_initialized = FALSE;

int DI_getId(char *pKey, DocId *docid)
{
	int ret = 0;	// for return value of function call
	ClientMsg clientmsg = {/*proctocolId*/"DITP", /*majorVer*/0, /*minorVer*/0, 
						/*check*/0, {""/*key*/, 0/*keyLen*/, 0/*method*/}
						/*ClientRequest*/ };
	ServerMsg servermsg = {/*stat*/0, /*method*/0, /*result*/0, /*check*/0};

	if (sock_initialized == FALSE) {
		sock_initialized = TRUE;
		ret = sb_run_udp_connect(&mSockfd, mIP, mPort);
		if (ret < 0) {
			error("cannot connect to server [%s:%s]", mIP, mPort);
			return DI_UNREACHABLE_SERVER;
		}
	}

	// fill in sending packet message
	clientmsg.session = (int32_t)time(NULL);
	clientmsg.mesg.nKeyLen = ((strlen(pKey)>MAX_DID_KEY_LEN)? 
									MAX_DID_KEY_LEN:strlen(pKey));
	memcpy(clientmsg.mesg.aKey, pKey, clientmsg.mesg.nKeyLen);
	clientmsg.mesg.nMethod = DITP_GET_ID;

	// send message
	ret = requestServer(mSockfd, &servermsg, &clientmsg);
	if (ret != SUCCESS) {
		error("cannot get response from server");
		return ret;
	}

	*docid = servermsg.result; //(DocId) servermsg.result;

	// servermsg.status can be DI_NOT_REGISTERED, DI_OLD_REGISTERED, ...
	// tsumari error ha 0 yori ookii kanoseimo arutte koto.
	return (int) servermsg.status; 
}

int DI_getNewId(char *pKey, DocId *docid, DocId *olddocid)
{
	int ret = 0;	// for return value of function call
	ClientMsg clientmsg = {/*proctocolId*/"DITP", /*majorVer*/0, /*minorVer*/0, 
						/*check*/0, {""/*key*/, 0/*keyLen*/, 0/*method*/}
						/*ClientRequest*/ };
	ServerMsg servermsg = {/*stat*/0, /*method*/0, /*result*/0, /*check*/0};

	if (sock_initialized == FALSE) {
		sock_initialized = TRUE;
		ret = sb_run_udp_connect(&mSockfd, mIP, mPort);
		if (ret < 0) {
			error("cannot connect to server [%s:%s]", mIP, mPort);
			return DI_UNREACHABLE_SERVER;
		}
	}

	// fill in the packet message
	clientmsg.session = (int32_t)time(NULL);
	clientmsg.mesg.nKeyLen = ((strlen(pKey)>MAX_DID_KEY_LEN)? 
								MAX_DID_KEY_LEN:strlen(pKey));
	memcpy(clientmsg.mesg.aKey, pKey, clientmsg.mesg.nKeyLen);
	clientmsg.mesg.nMethod = DITP_GET_NEW_ID;

	ret = requestServer(mSockfd, &servermsg, &clientmsg);
	if (ret != SUCCESS) {
		error("cannot send message to server");
		return ret;
	}
	
	*docid = (DocId) servermsg.result;
	*olddocid = (DocId) servermsg.olddid;

	return (int) servermsg.status;
}

int DI_getNumDoc(DocId *num_of_doc)
{
	int ret = 0;	// to temporary save return value of function
	ClientMsg clientmsg = {/*proctocolId*/"DITP", /*majorVer*/0, /*minorVer*/0, 
						/*check*/0, {""/*key*/, 0/*keyLen*/, 0/*method*/}
						/*ClientRequest*/ };
	ServerMsg servermsg = {/*stat*/0, /*method*/0, /*result*/0, /*check*/0};

	if (sock_initialized == FALSE) {
		sock_initialized = TRUE;
		ret = sb_run_udp_connect(&mSockfd, mIP, mPort);
		if (ret < 0) {
			error("cannot connect to server [%s:%s]", mIP, mPort);
			return DI_UNREACHABLE_SERVER;
		}
	}

	// fill in the packet message
	clientmsg.session = (int32_t)time(NULL);
	clientmsg.mesg.nMethod = DITP_GET_NUM_DOC;

	ret = requestServer(&servermsg, &clientmsg);
	if (ret != SUCCESS) {
		error("cannot send message to server");
		return DI_UNREACHABLE_SERVER;
	}

	*num_of_doc= (DocId) servermsg.result;

	return (int) servermsg.status;
}

static int requestServer(int mSockfd, ServerMsg *servermsg, ClientMsg *clientmsg)
{
	// send message to server
	if (sb_run_udp_send(mSockfd, clientmsg, sizeof(ClientMsg), UDP_TIMEOUT) == -1) {
		return FAIL;
	}

	// receive server message
	if (sb_run_udp_recv(mSockfd, servermsg, sizeof(ServerMsg), UDP_TIMEOUT) == -1) {
		return FAIL;
	}

	if (servermsg->session != clientmsg->session) {
		return FAIL;
	}

    return SUCCESS;
}
static int init()
{
	int port=0;
	port = assignSoftBotPort(__FILE__,PORT_ID_DID);
	snprintf(mPort,SHORT_STRING_SIZE,"%d",port);
	return SUCCESS;
}
/*****************************************************************************/
//XXX: port is assigned by server (by assignSoftBotPort in server.c)
static void get_ip(configValue v) {
	strncpy(mIP, v.argument[0], SHORT_STRING_SIZE);
}

static void register_hooks(void)
{
	sb_hook_client_get_docid(DI_getId,NULL,NULL,HOOK_MIDDLE);
	sb_hook_client_get_new_docid(DI_getNewId,NULL,NULL,HOOK_MIDDLE);
	sb_hook_client_get_last_docid(DI_getNumDoc,NULL,NULL,HOOK_MIDDLE);
}

static config_t config[] = {
	CONFIG_GET("Connect", get_ip, 1, "did server ip"),
	{NULL}
};

module did_client_module = {
	STANDARD_MODULE_STUFF,
	config,				/* config */
	NULL,				/* registry */
	init,               /* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks		/* register hook api */
};
