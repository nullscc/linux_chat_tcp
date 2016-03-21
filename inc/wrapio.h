#ifndef _WRAPIO_H_
#define _WRAPIO_H_
#include "wrapio.h"
#include <poll.h>
#include "chat.h"

static int	read_cnt;
static char	*read_ptr;
static char	read_buf[MAXLINE];

ssize_t writen(int fd, const void *buf, size_t count);

ssize_t Writen(int fd, const void *buf, size_t count);

char *Fgets(char *s, int size, FILE *stream);

int Fputs(const char *s, FILE *stream);

static ssize_t my_read(int fd, char *ptr);

ssize_t readline(int fd, void *vptr, size_t maxlen);

ssize_t Readline(int fd, void *ptr, size_t maxlen);

ssize_t Read(int fd, void *ptr, size_t nbytes);

ssize_t Sockread(int fd, void *ptr, size_t nbytes);

int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

int Poll(struct pollfd *fds, nfds_t nfds, int timeout);

void clearbuf(int flag);

void printf_flush(char * const str);

int is_dir_exist(const char *path);

#endif
