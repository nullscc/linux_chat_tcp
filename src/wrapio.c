#include "wrapio.h"
#include <poll.h>
#include <unistd.h>
#include "chat.h"

/********************************
 *功能：保证能向描述符fd写入count个字符
 ******************************/
ssize_t writen(int fd, const void *buf, size_t count)
{
	ssize_t ntowrite;
	ssize_t n;
	const char *pbuf;

	ntowrite = count;
	pbuf = buf;
again:
	if( (n = write(fd, pbuf, ntowrite)) <= 0 )
	{
		if ( (errno == EINTR) && (n < 0) )
		{
			ntowrite -= n;
			pbuf += n;
			goto again;
		}
		else
			return -1;
	}
	return count;	
}

/********************************
 *功能：writen的包裹函数
 ******************************/
ssize_t Writen(int fd, const void *buf, size_t count)
{	
	if( writen(fd, buf, count) != count )
	{
		perror("writen error");
		return -1;
	}
	return count;
}

/********************************
 *功能：fgets的包裹函数
 ******************************/
char *Fgets(char *s, int size, FILE *stream)
{
	char *str;
	if( (str = fgets(s, size, stream))==NULL )
		{
			perror("fgets error");
			return NULL;
		}
	return str;
}


/********************************
 *功能：fputs的包裹函数
 ******************************/
int Fputs(const char *s, FILE *stream)
{
	int ret;
	if( (ret = fputs(s, stream))==EOF )
	{
		perror("fgets error");
		return -1;
	}
	return ret;
}

static int	read_cnt;
static char	*read_ptr;
static char	read_buf[MAXLINE];

/********************************
 *功能：使用自己定义的缓冲区的read函数，这样好把控
 ******************************/
static ssize_t
my_read(int fd, char *ptr)
{

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return(1);
}

/********************************
 *功能：高效的从描述符fd中读取一行
 ******************************/
ssize_t readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}

/********************************
 *功能：readline的包裹函数
 ******************************/
ssize_t Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t		n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
	{
		perror("readline error");
		return -1;
	}
	return(n);
}

/********************************
 *功能：Read的包裹函数
 ******************************/
ssize_t Read(int fd, void *ptr, size_t nbytes)
{
	ssize_t		n;

	if ( (n = read(fd, ptr, nbytes)) == -1)
	{
		perror("read error");
		return -1;
	}
	return(n);
}

/********************************
 *功能：当read返回0，表示对端结束了，打印一个消息来提醒客户端
 ******************************/
ssize_t Sockread(int fd, void *ptr, size_t nbytes)
{
    ssize_t		n;

    if ( (n = Read(fd, ptr, nbytes)) == 0)
    {
        printf(LIGHT_RED"server terminted prematurely\n"COLOR_NONE);
        return -1;
        exit(1);
    }
    return(n);
}

/********************************
 *功能：select的包裹函数
 ******************************/
int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	int n;
	if( (n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0)
	{
		perror("select error");
		return -1;
	}
	return n;
}

/********************************
 *功能：poll的包裹函数，推荐使用select
 ******************************/
int Poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int n;
	if( (n = poll(fds, nfds, timeout)) < 0)
	{
		perror("select error");
		return -1;
	}
	return n;
}

/********************************
 *功能：清空标准输入的缓冲区
 ******************************/
void clearbuf(int flag)
{
    if(flag)
        fflush(stdin); //不推荐，有些编译器中fflush对stdin的行为未定义这里我暂时偷懒下，使用自定义的缓冲区可能会比较好
    else
    {
        char ch;
        while( (ch=getchar())!='\n' && ch!=EOF); //这种办法会阻塞
    }
}

/********************************
 *功能：清空标准输出的缓冲区，并理解打印缓冲区的内容
 ******************************/
void printf_flush(char * const str)
{
    printf("%s", str);
    fflush(stdout); //加上fflush强制刷新缓冲区
}

/********************************
 *功能：判断路径中的文件夹是否存在
 ******************************/
int is_dir_exist(const char *path)
{
    struct stat dirstat;

    if(path == NULL)
        return 0;
    if(stat(path, &dirstat) == -1)
    {
        return 0;
    }
    else
    {
        if(S_ISDIR(dirstat.st_mode)) /* 若path表示的路径的确存在，那么判断一下是文件还是目录，如果是文件，那么目录还是不存在的 */
            return 1;
        else return 0;
    }
    return 0;

}
