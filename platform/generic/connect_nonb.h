/*
 * $Id$
 */
#ifndef _CONNECT_NONB_H_
#define _CONNECT_NONB_H_ 1

#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "softbot.h"

SB_DECLARE(int) connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec);

#endif
