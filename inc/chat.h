#ifndef _CHAT_H_
#define _CHAT_H_
#include "chat.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> //struct sockaddr_in在这个头文件里面
#include <errno.h>

#define PRINTF_RED(msg, arg...) \
    //printf("\033[1;31m" "%s %s(%d) " msg "\033[m\n", __FILE__, __FUNCTION__, __LINE__ , ##arg)

#define PRINTF_DESTINATION() \
    //printf("\033[1;31m" "%s %s(%d) " "\033[m\n", __FILE__, __FUNCTION__, __LINE__)

#define DEBUG_LONG(msg, arg...) \
    //printf( "%s %s(%d) " msg , __FILE__, __FUNCTION__, __LINE__ , ##arg)

#define DEBUG(msg, arg...) \
    //printf( msg , ##arg)

#define OPEN_MAX 256
#define	MAXLINE		4096	/* max text line length */

#define TRUE 1
#define FALSE 0

#define SA struct sockaddr

#define SRVLOGDIR "/etc/chat/log/"
#define CHATLOGDIR "/etc/chat/chatlog/"

#define COLOR_NONE         "\033[m"
#define RED          "\033[0;32;31m"
#define LIGHT_RED    "\033[1;31m"
#define GREEN        "\033[0;32;32m"
#define LIGHT_GREEN  "\033[1;32m"
#define BLUE         "\033[0;32;34m"
#define LIGHT_BLUE   "\033[1;34m"
#define DARY_GRAY    "\033[1;30m"
#define CYAN         "\033[0;36m"
#define LIGHT_CYAN   "\033[1;36m"
#define PURPLE       "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN        "\033[0;33m"
#define YELLOW       "\033[1;33m"
#define LIGHT_GRAY   "\033[0;37m"
#define WHITE        "\033[1;37m"

enum Option
{
    NONEOPTION,
    REGISTER,
    LOGIN,
    COMMAND,
    PRIVATEMSG,
    SENDMSG,
    MAXOPTION
};

struct chat_info
{
	char UserName[25];
    char UserPasswd[20];
    char RealTime[10];
    enum Option flag;
    char cmd[20];
    char PrvName[25];
	char msg[MAXLINE];
};

struct user_info
{
    struct sockaddr_in cliaddr;
    char cliname[25];
};



#define	max(a,b)	((a) > (b) ? (a) : (b))

#endif
