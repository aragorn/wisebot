/* $Id$ */

#ifndef __MESSAGE_H
#define __MESSAGE_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>

#define __SUPPRESS__MESSAGE__

#define message_send(dst, d, l) _message_send(dst, d, l, __FUNCTION__)
// probably should stick _t on or something
typedef union message message;
typedef int           msg_addr;
typedef enum msg_type msg_type;

enum  msg_type {
		msg_no_message,
		msg_ping,   msg_pong,   msg_hello,   msg_goodbye,   msg_welcome,   msg_shm_request,   msg_shm_notify, msg_xmlrpc, msg_func_call, msg_func_result, 
		msg_notify,
		msg_fd_msg_start, msg_fd_notify, msg_stream, msg_fd_msg_end
};

// maintain this in msg.c v--
extern const char *msg_name[];
/*{ "ping", "pong", "hello", "goodbye", "welcome", "shm_request", "shm_notify", "xmlrpc", "fd_notify" };*/

struct msg_base        { unsigned short length; msg_type op; };

struct msg_ping        { unsigned short length; msg_type op; unsigned int seq; };
struct msg_pong        { unsigned short length; msg_type op; unsigned int seq; };
struct msg_hello       { unsigned short length; msg_type op; char name[64];    };
struct msg_goodbye     { unsigned short length; msg_type op; };
struct msg_welcome     { unsigned short length; msg_type op; char name[64];    };
struct msg_shutdown    { unsigned short length; msg_type op; };

struct msg_xmlrpc      { unsigned short length; msg_type op; unsigned int data_length; char data[0]; };
struct msg_func_call   { unsigned short length; msg_type op; int id; unsigned int data_length; char data[0]; };
struct msg_func_result { unsigned short length; msg_type op; int id; int rv; unsigned int data_length; char data[0]; };

struct msg_shm_request { unsigned short length; msg_type op; };
struct msg_shm_notify  { unsigned short length; msg_type op; void *addr; size_t size; void *registry; };
struct msg_fd_notify   { unsigned short length; msg_type op; int fd; msg_addr src_fd; };
struct msg_stream      { unsigned short length; msg_type op; int fd; msg_addr src_fd; unsigned char data[0]; };

struct msg_notify      { unsigned short length; msg_type op; int path_length; char path[0]; };

union message {
	unsigned short         length;
	struct msg_base        base;
	struct msg_ping        ping;
	struct msg_pong        pong;
	struct msg_hello       hello;
	struct msg_goodbye     goodbye;
	struct msg_welcome     welcome;
	struct msg_shutdown    shutdown;

	struct msg_xmlrpc      xmlrpc;
	struct msg_func_call   func_call;
	struct msg_func_result func_result;

	struct msg_shm_request shm_request;
	struct msg_shm_notify  shm_notify;

	struct msg_fd_notify   fd_notify;
	struct msg_stream      msg_stream;

	struct msg_notify      msg_notify;
};

// public interface
//
typedef int (*subhandler_t)(msg_addr self, msg_addr src, message *msg, int length);

#if 0 
int message_open();
int message_close();
int message_send(msg_addr dst, message *data, int size);
int message_send_fd(msg_addr dst, message *data, int length, int fd);
int message_broadcast(message *data, int size);
int message_receive(msg_addr *src, message *data, int size);
int message_identity();

int message_install_general_handler(subhandler_t subhandler);
int message_install_handler(msg_type op, subhandler_t subhandler);

int message_install_general_filter(subhandler_t subhandler);
int message_install_filter(msg_type op, subhandler_t subhandler);

// conveniences

LAZY_FUNCTION("message/method/open",                     , int, message_open,      (),                                           ());
LAZY_FUNCTION("message/method/close",                    , int, message_close,     (),                                           ());
LAZY_FUNCTION("message/method/send",                     , int, message_send,      (msg_addr dst, message *data, int size),      (dst, data, size));
LAZY_FUNCTION("message/method/broadcast",                , int, message_broadcast, (message *data, int size),                    (data, size));
LAZY_FUNCTION("message/method/identity",                 , int, message_identity,  (),                                           ());
LAZY_FUNCTION("message/method/install_general_handler",  , int, message_install_general_handler, (subhandler_t subhandler),      (subhandler));
LAZY_FUNCTION("message/method/install_handler",          , int, message_install_handler, (msg_type op, subhandler_t subhandler), (op, subhandler));
LAZY_FUNCTION("message/method/install_general_filter",   , int, message_install_general_filter, (subhandler_t subhandler),       (subhandler));
LAZY_FUNCTION("message/method/install_filter",           , int, message_install_filter, (msg_type op, subhandler_t subhandler),  (op, subhandler));
#else

#  ifdef __SUPPRESS__MESSAGE__
#  define message_open()      (SUCCESS)
#  define message_close()     (SUCCESS)
#  define _message_send(dst, d, s, f)   (SUCCESS)
#  define message_send_fd(dst, d, l, f) (SUCCESS)
#  define message_broadcast(d, s)       (SUCCESS)
#  define message_receive(h)            (SUCCESS)
#  define message_identity()            (SUCCESS)
#  define message_install_general_handler(h) (SUCCESS)
#  define message_install_handler(op, h)     (SUCCESS)
#  define message_install_general_filter(h)  (SUCCESS)
#  define message_install_filter(op, h)      (SUCCESS)
#  define with_message_handler(op, h, v)     (SUCCESS)

#  else
SB_DECLARE(int) message_open();
SB_DECLARE(int) message_close();
SB_DECLARE(int) _message_send(msg_addr dst, message *data, int size, char *func);
SB_DECLARE(int) message_send_fd(msg_addr dst, message *data, int length, int fd);
SB_DECLARE(int) message_broadcast(message *data, int size);
SB_DECLARE(int) message_receive(int (*handler)(msg_addr src, message *msg, int length));
SB_DECLARE(int) message_identity();

SB_DECLARE(int) message_install_general_handler(subhandler_t subhandler);
SB_DECLARE(int) message_install_handler(msg_type op, subhandler_t subhandler);

SB_DECLARE(int) message_install_general_filter(subhandler_t subhandler);
SB_DECLARE(int) message_install_filter(msg_type op, subhandler_t subhandler);

SB_DECLARE(int) with_message_handler(msg_type op, \
					int (*subhandler)(int self, msg_addr src, message *msg, int length), int (*body)(void));
#  endif // __SUPRESS__MESSAGE__
#endif // __MESSAGE_C

#endif // __MESSAGE_H
