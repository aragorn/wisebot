/* $Id$ */

/////////////////////////////////////
//      service module             //
/////////////////////////////////////


/**************************
1. What is this?
 네트웍관련 모듈
 socket create, bind, listen, connect

2. Where is gone BEEP?
 BEEP의 System dependency가 크고 낮은 버전임을 고려 해서
 next용으로 비슷하게 개발

3. Is there a similiar module already developed?
 있다. beepapi module

4. What is Beep api?
 BEEP을 쉽게사용하기 위해서 BEEP위에 덧씌운 모듈이다.
 BEEP은 libxml2 library를 사용한다.

 BEEPAPI의 interface를 보자.
*/

int BPAPI_init (Connect* connect);
int BPAPI_listen (char* address, int port, Connect* connect);
int BPAPI_connect (char* address, int port, Connect* connect);
int BPAPI_shutdown (Connect* connect);
int BPAPI_loadProtocol (char* name, char* dtdfile, 
		                       Protocol* protocol, Connect* connect,
							   char* majorVersion, char* minorVersion);
int BPAPI_setCallback (char* methodname, BeepServerSideCallbackFunction func,
		                      Protocol* protocol);
int BPAPI_channelOpen(Connect* connect, Protocol* protocol);
int BPAPI_channelClose(Protocol* protocol);

int BPAPI_initMessage (char* method, Message* message, Protocol* protocol);
int BPAPI_addItem (char* item, ItemType type, ItemData buf, int size, Message* message);
int BPAPI_getItem (char* item, ItemType *type, ItemData* buf, int *size, Message* message);

int BPAPI_sendMessage (Protocol* protocol, Message*in, Message* out);
int BPAPI_freeMessage (Message* message);

// extended interfaces
int BPAPI_getItemList (char *name, Item *items, Message *message);
int BPAPI_getItemFromGroup (char *name, Item *item, Item *group);
int BPAPI_getItemListFromGroup (char *name, Item *item, Item *group);

int BPAPI_makeItem (char *name, ItemType type, ItemData data, int size,
		                   Item *item, Message *message);
int BPAPI_addItemToGroup (Item *item, Item *group);
int BPAPI_addItemToMessage (Item *item, Message *message);

/*
5. What should I do?
 apply beepapi interface to next tcp protocol module

6. 
****************************/
