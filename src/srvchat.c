#include "chat.h"

/*****************************
 *服务端程序开始处，调用方式：“./srvchat 端口”
 ***************************/

int main(int argc, char **argv)
{
    int listenfd;
    struct sockaddr_in srvaddr;
    struct stat dirstat;

    if(argc != 2)
    {
        printf("Usage:%s <Port>\n", argv[0]);
        return -1;
    }

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(atoi(argv[1]));
	srvaddr.sin_addr.s_addr = INADDR_ANY;

	Bind(listenfd, (SA *)&srvaddr, sizeof(srvaddr));

	Listen(listenfd, 100);

    if ( 0 != stat("/etc/chat",&dirstat))   //If failed to get the status of this directory
    {
         if (ENOENT == errno) //If folder  not exist
         {
            mkdir("/etc/chat", S_IRWXU);
         }
    }

    str_echo(listenfd);
	
	return 0;
}

