/* $Id$ */

#define __MESSAGE_C


#include <dirent.h>
#include <errno.h>
#include <stdlib.h>

#include "softbot.h"

//#include "registry.h" XXX: commented out temporarily
#include "shm.h"
#include "message.h"

#ifndef __SUPPRESS__MESSAGE__

// FIXME v-- out of date
const char *msg_name[] = { "ping", "pong", "hello", "goodbye", "welcome", "shutdown", "shm_request", "shm_notify", "xmlrpc", "func_call",
		"fd_msg_start", "fd_notify", "fd_msg_end" };

typedef struct message_handler message_handler;

struct message_handler {
	message_handler *next;
	int  (*handler)   (message_handler *handler, int self, msg_addr src, message *msg, int length);
	int  (*subhandler)(msg_addr self, msg_addr src, message *msg, int length);
	msg_type op;
};

static int port;
static message_handler *handlers;

static int general_handler(message_handler *handler, int self, msg_addr src, message *msg, int length);
static int general_filter(message_handler *handler, int self, msg_addr src, message *msg, int length);
static int dispatch_handler(message_handler *handler, msg_addr self, msg_addr src, message *msg, int length);
static int dispatch_filter(message_handler *handler, int self, msg_addr src, message *msg, int length);
static int message_maintain_ports(int error, struct sockaddr_un *addr);
static void process_messages(int ignore);

static int ping_handler(int self, msg_addr src, message *msg, int length);

int message_open()
{
	int fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	int pid = getpid();
	struct sockaddr_un addr = { AF_UNIX, "" };
	unsigned long bufsize = 1024*128; /* 128k */
	struct sigaction action;

	action.sa_handler = process_messages;
	action.sa_flags   = SA_RESTART;
	sigfillset(&action.sa_mask);
	
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof bufsize);

	sprintf(addr.sun_path, "msg:%05d", pid);
	unlink(addr.sun_path);
	bind(fd, (struct sockaddr *)&addr, sizeof addr);

	sigaction(SIGIO, &action, NULL);

#ifdef AIX5
	fcntl(fd, F_SETFL,  FASYNC|FNONBLOCK);
#else
	fcntl(fd, F_SETFL,  O_ASYNC|O_NONBLOCK);
#endif
	fcntl(fd, F_SETOWN, pid);

	port = fd;

	atexit((void *)message_close);

	return 1;
}


int message_close()
{
	struct sockaddr_un addr;
	sprintf(addr.sun_path, "msg:%05d", message_identity());
	unlink(addr.sun_path);

	return 0;
}


int _message_send(msg_addr dst, union message *data, int length, char *func)
{
	struct sockaddr_un addr = { AF_UNIX, "msg:00000" };
	int r;

	data->length = length;

	/*fprintf(stderr, "sending message with length = %hu, op:%d\n",
								data->length, data->base.op);*/

	sprintf(&addr.sun_path[4], "%05d", dst);

	// special cases
	if (data->base.op > msg_fd_msg_start && data->base.op < msg_fd_msg_end) {
		int r;
		// see if we need to manufacture the fd's first
		// msg_stream setup
		if (data->base.op == msg_stream) {
			// currently we only support local stream openings
			// but this should support opening streams to other
			// machines transparently, later on.
			// (Which is why this code is here)
			int fds[2];
	
			if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
				perror("socketpair: ");
				return 0;
				}
	
			data->msg_stream.fd      = fds[0]; // also data->fd_notify.fd
			data->msg_stream.src_fd  = fds[1];
			}
		r = message_send_fd(dst, data, length, data->fd_notify.fd);
		// msg_stream cleanup
		if (data->base.op == msg_stream) {
			if (r == 0)
				close(data->msg_stream.src_fd);
			close(data->msg_stream.fd);
			}
		return r;
	} else {
		// default case
		r = sendto(port, data, length, 0, (struct sockaddr *)&addr, sizeof addr);
		}

		if (r == -1) message_maintain_ports(errno, &addr);

	return r;
}

int message_send_fd(msg_addr dst, union message *data, int length, int fd)
{
    struct iovec vector;
    struct msghdr msg;
    struct cmsghdr *cmsg;
	struct sockaddr_un addr = { AF_UNIX, "msg:00000" };
	int r;

	int xmit = socket(PF_UNIX, SOCK_DGRAM, 0);
	
	if (xmit == -1) {
		perror("opening socket to send fd");
		return 0;
		}

	data->length = length;

    /* Put together the first part of the message. Include the
       file name iovec */

    vector.iov_base = data;
    vector.iov_len  = length;

    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov     = &vector;
    msg.msg_iovlen  = 1;

    /* Now for the control message. We have to allocate room for
       the file descriptor. */
    cmsg = alloca(sizeof(struct cmsghdr) + sizeof(port));
    cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(port);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;


    /* copy the file descriptor onto the end of the control
       message */
    memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));

    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg->cmsg_len;

	// I wonder if this will work...
	sprintf(&addr.sun_path[4], "%05d", dst);

	connect(xmit, (struct sockaddr *)&addr, sizeof addr);
    r = sendmsg(xmit, &msg, 0);
	close(xmit);

	if (r == -1) message_maintain_ports(errno, &addr);

    return 0;
}


#if 0
int message_receive(msg_addr *src, void *data, int length)
{
	struct sockaddr_un addr;
	int size = sizeof addr;
	int r;

	r = recvfrom(port, data, length, 0, (struct sockaddr *)&addr, &size);

	if (r == -1) {
		if (errno != EAGAIN) {
			perror("message_send: ");
			}
		return 0;
	} else if (!strncmp("msg:", addr.sun_path, 4) && strlen(addr.sun_path) == 9)
		*src = atoi(&addr.sun_path[4]);

	return r;
}
#endif

#if 0
// all this just to be able to receive file-descriptors :]
int message_receive(msg_addr *src, message *data, int length)
{
	struct sockaddr_un addr;
	int size = sizeof addr;

    struct iovec vector;
    struct msghdr msg;
    struct cmsghdr *cmsg;
	int r;

    /* set up the iovec for the file name */
    vector.iov_base = data;
    vector.iov_len  = length;

    /* the message we're expecting to receive */

    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov     = &vector;
    msg.msg_iovlen  = 1;

    /* dynamically allocate so we can leave room for the file
       descriptor */
    cmsg = alloca(sizeof(struct cmsghdr) + sizeof(port));
    cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(port);
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg->cmsg_len;

	recvfrom(port, data, 0, MSG_PEEK, (struct sockaddr *)&addr, &size);

	if (!strncmp("msg:", addr.sun_path, 4) && strlen(addr.sun_path) == 9)
		*src = atoi(&addr.sun_path[4]);

    r = recvmsg(port, &msg, 0);

	if (r == -1) {
		if (errno != EAGAIN) {
			perror("message_send: ");
			}
		return 0;
		}

	// horrible case specific code
	{
		union message *msg = (union message *)data;
		// patch the message to have the proper fd
		if (msg->base.op > msg_fd_msg_start && msg->base.op < msg_fd_msg_end) {
    		memcpy(&msg->fd_notify.fd, CMSG_DATA(cmsg), sizeof(msg->fd_notify.fd));
			}
	}

    return vector.iov_len;
}
#endif

int message_receive(int (*handler)(msg_addr src, message *msg, int length))
{
	// look at refactoring this out
	struct sockaddr_un addr;
	int size = sizeof addr;

    struct iovec vector;
    struct msghdr msg;
    struct cmsghdr *cmsg;

	msg_addr src = 0;
	unsigned short length;
	int r;

	// stupid linux doesn't support MSG_TRUNC, remember to write a nasty letter
	// in the meantime we prefix the packet with a 2-byte length and use that...
	r = recvfrom(port, &length, sizeof length, MSG_PEEK|MSG_TRUNC, (struct sockaddr *)&addr, &size);

	if (r == -1) {
		if (errno != EAGAIN) {
			perror("message_receive: ");
			}
		// there was no message there...
		return 0;
		}

	// length should be the length of the message, probably
	DEBUG("peeked length = %d\n", length);

	if (!strncmp("msg:", addr.sun_path, 4) && strlen(addr.sun_path) == 9)
		src = atoi(&addr.sun_path[4]);

	{
		char data[length];

    	vector.iov_base = data;
    	vector.iov_len  = length;

    	/* the message we're expecting to receive */

    	msg.msg_name    = NULL;
    	msg.msg_namelen = 0;
    	msg.msg_iov     = &vector;
    	msg.msg_iovlen  = 1;

    	/* dynamically allocate so we can leave room for the file
       	descriptor */
    	cmsg = alloca(sizeof(struct cmsghdr) + sizeof(port));
    	cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(port);
    	msg.msg_control = cmsg;
    	msg.msg_controllen = cmsg->cmsg_len;

    	r = recvmsg(port, &msg, 0);

		if (r == -1) {
			if (errno != EAGAIN) {
				perror("message_receive: ");
				}
			// there was no message there...
			return 0;
			}

		// horrible case specific code
		{
			union message *msg = (union message *)data;
			// patch the message to have the proper fd
			if (msg->base.op > msg_fd_msg_start && msg->base.op < msg_fd_msg_end) {
    			memcpy(&msg->fd_notify.fd, CMSG_DATA(cmsg), sizeof(msg->fd_notify.fd));
				}
		}

		handler(src, (message *)data, length);
		return length;
	}
}

/*

...

void foo()
{
	int handler(msg_addr src, message *msg, int length) {
		fprintf(stderr, "I received a message of length %d\n", length);
		return 1;
		}

	message_receive(handler);
}

*/

// can't broadcast fd's - FIXME
int message_broadcast(message *data, int length)
{
	struct sockaddr_un addr = { AF_UNIX, "msg:00000" };
	struct dirent *entry;
	DIR *dir = opendir(".");
	int count = 0;
	int identity = message_identity();

	while((entry = readdir(dir))) {
		// only send if the name is of the correct form 
		//  - actually we should check that it is a socket, I guess.
		// and it isn't our own
		if (!strncmp(entry->d_name, "msg:", 4) 
			&& strlen(entry->d_name) == 9 && (atoi(&entry->d_name[4]) != identity)) {
			strcpy(addr.sun_path, entry->d_name);
			data->length = length;
			if (sendto(port, data, length, 0, (struct sockaddr *)&addr, sizeof addr) == -1) {
				message_maintain_ports(errno, &addr);
			} else  {
				count++;
			}
		}
	}

	closedir(dir);

	return count;
}

int message_identity()
{
	return getpid();
}

int message_broadcast_ping(unsigned int seq)
{ 
	struct msg_ping msg = { msg_ping, seq }; 
	return message_broadcast((message *)&msg,  sizeof msg); 
}


int message_send_pong(msg_addr dst, unsigned int seq)
{ 
	struct msg_pong msg = { msg_pong, seq }; 
	return message_send(dst, (message *)&msg,  sizeof msg); 
}



int message_send_fd_notify(msg_addr dst, int fd)
{ 
	struct msg_fd_notify msg = { msg_fd_notify, fd, fd };
	return message_send(dst, (message *)&msg, sizeof(struct msg_fd_notify)); 
}

int message_open_stream(msg_addr dst, unsigned char *data, int length)
{
	unsigned char store[sizeof(struct msg_stream)+length];
	message *msg = (message *)&store[0];

	memcpy(msg->msg_stream.data, data, length);

	msg->base.op = msg_stream;

	if (message_send(dst, msg, sizeof store)) {
		// the message system will patch this packet for me
		// we do it like this so that the message transport
		// layer can determine which kind of socket (or pipe)
		// we need. (ie, AF_UNIX, AF_INET, PIPE, etc)
		return msg->msg_stream.src_fd;
		}

	return -1;
}

static int general_handler(message_handler *handler, int self, msg_addr src, message *msg, int length)
{
	handler->subhandler(self, src, msg, length);

	return 1;
}

static int general_filter(message_handler *handler, int self, msg_addr src, message *msg, int length)
{
	return handler->subhandler(self, src, msg, length);
}

int message_install_general_handler(int (*subhandler)(msg_addr self, msg_addr src, message *msg, int length))
{
	message_handler *obj = sb_malloc(sizeof(message_handler));

	obj->subhandler = subhandler;
	obj->handler    = general_handler;
	obj->next       = handlers;
	obj->op         = msg_no_message;

	handlers = obj;

	return 1;
}

int message_install_general_filter(int (*handler)(int self, msg_addr src, message *msg, int length))
{
	message_handler *obj = sb_malloc(sizeof(message_handler));

	obj->subhandler = handler;
	obj->handler    = general_handler;
	obj->next       = handlers;
	obj->op         = msg_no_message;

	handlers = obj;

	return 1;
}

int message_install_handler(msg_type op, int (*subhandler)(msg_addr self, msg_addr src, message *msg, int length))
{
	message_handler *obj = sb_malloc(sizeof(message_handler));

	obj->handler    = dispatch_handler;
	obj->subhandler = subhandler;
	obj->next       = handlers;
	obj->op         = op;

	handlers = obj;

	return 1;
}

int message_uninstall_handler(msg_type op, int (*subhandler)(int self, msg_addr src, message *msg, int length))
{
	message_handler *handler, **patch;

	for(patch=&handlers,handler=handlers;handler;patch=&handler->next,handler=handler->next) {
		if ((handler->op == op) && (handler->subhandler == subhandler)) {
			*patch = handler->next;
			sb_free(handler);
			return 1;
			}
		}

	return 0;
}

int message_install_filter(msg_type op, int (*subhandler)(int self, msg_addr src, message *msg, int length))
{
	message_handler *obj = sb_malloc(sizeof(message_handler));

	obj->handler    = dispatch_filter;
	obj->subhandler = subhandler;
	obj->next       = handlers;
	obj->op         = op;

	handlers = obj;

	return 1;
}

int message_uninstall_filter(msg_type op, int (*subhandler)(int self, msg_addr src, message *msg, int length))
{
	return message_uninstall_handler(op, subhandler);
}

int with_message_handler(msg_type op, int (*subhandler)(int self, msg_addr src, message *msg, int length), int (*body)(void))
{
	int ret = 0;

	if (message_install_handler(op, subhandler)) {
		ret = body();
		if (!message_uninstall_handler(op, subhandler))
			return 0;
		}
	return ret;
}

/* example of with_message_handler for blocking message reception
{
	int received = 0;
	int handler(int self, msg_addr src, message *msg, int length) {
		...
		// was it what I expected?
		received  = 1;
		}
	int body(void) {
		while(!received)
			pause();
		}
	with_message_handler(msg_xmlrpc, handler, body);
}
*/

static int dispatch_handler(message_handler *handler, msg_addr self, msg_addr src, message *msg, int length)
{
	if (msg->base.op == handler->op)
		handler->subhandler(self, src, msg, length);

	return 1;
}

static int dispatch_filter(message_handler *handler, int self, msg_addr src, message *msg, int length)
{
	if (msg->base.op == handler->op)
		return handler->subhandler(self, src, msg, length);

	return 1;
}

static int message_maintain_ports(int error, struct sockaddr_un *addr)
{
	switch(error) {
		case ECONNREFUSED:
			// someone left a socket lying about - remove it
			error("port maintenance: found a dead port <%s> - removing it.", 
							addr->sun_path);
			unlink(addr->sun_path);
			return 1;
		case ENOENT:
			error("port maintenance: no such file[%s]", addr->sun_path);
			return 1;
		default:
			error("pid[%d] port maintenance: %s", getpid(), strerror(errno));
/*			info("EBADF:%d, EMSGSIZE:%d, EAGAIN:%d, ENOBUFS:%d, EINTR:%d, ENOMEM:%d, EPIPE:%d, ENOENT:%d",
				EBADF, EMSGSIZE, EAGAIN, ENOBUFS, EINTR, ENOMEM, EPIPE,ENOENT);*/
		}

	return 0;
}

static void process_messages(int ignore)
{
	int identity;


	identity = message_identity();

	{
		int handler(msg_addr src, message *msg, int length) {
			message_handler *handler;
			for(handler=handlers;handler;handler=handler->next) {
				if (!handler->handler(handler, identity, src, msg, length)) {

					break;
				}
			}
			return length;
		}

		{ int n;
			do {
				n = message_receive(handler);
				DEBUG("message_receive return [%d]", n);
			}while(n);
		}
	}
}

static int ping_handler(int self, msg_addr src, message *msg, int length)
{
	if (src != self)
		message_send_pong(src, msg->ping.seq);
	return 1;
}

//XXX: commented out temporarily
#if 0
// make this less ugly --v - TODO
#define publish(name, func) class(methods)->insert(methods, # name, sizeof # name -1, make_function_node(methods, LOCAL_CLASS, # func, func))

node *ensure(node *self, const char *path, int length)
{
	fprintf(stderr, "shared->ensure(%s, %d)\n", path, length);

	if (!strncmpn(path, length, "method", 6)) {
		node *methods = make_root_dictionary_node(LOCAL_CLASS);
		publish(open,                    message_open);
		publish(close,                   message_close);
		publish(send,                    message_send);
		publish(broadcast,               message_broadcast);
		publish(identity,                message_identity);
		publish(install_general_handler, message_install_general_handler);
		publish(install_handler,         message_install_handler);
		publish(install_general_filter,  message_install_general_filter);
		publish(install_filter,          message_install_filter);
		return methods;
		}

	return NULL;
}

void iterate(node *self, const char *path, int length, void (*iterator)(const char *path))
{
	strndupncatpathapply(path, length, "method",  6, iterator);
}

void _init()
{
	message_install_handler(msg_ping,  ping_handler);
}
#endif // __SUPPRESS__MESSAGE__

#endif
