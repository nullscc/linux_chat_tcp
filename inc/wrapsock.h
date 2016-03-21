#ifndef _WRAPSOCK_H_
#define _WRAPSOCK_H_
#include "wrapsock.h"
#include <netinet/in.h>

int Socket(int domain, int type, int protocol);

int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int Listen(int sockfd, int backlog);

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int isvalidip(char *ip);

#endif
