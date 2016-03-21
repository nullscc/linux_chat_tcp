#include "chat.h"
#include <netdb.h>
#include <stdlib.h>

/**************************
 *功能：客户端测试程序运行起始处,是为了测试服务器在多个客户连接情况下的响应速度
 *     程序运行方法：
 *          clichat <IP地址/域名> <端口> <测试个数>
 *     测试个数需要少于1019
 *************************/
int main(int argc, char**argv)
{
    int sockfd;
    struct sockaddr_in srvaddr, cliaddr;
    struct chat_info cli_info;
    int n, i;
    int clifd[FD_SETSIZE] = {0};
    struct hostent *h;
    char ipstr[16];
    fd_set sel_rdset;
    int nready = 0;
    if(ENABLE_TEST)
    {
        printf("ENABLE_TEST\n");
    }
    else
    {
        printf("DISABLE_TEST\n");
        printf("Please Goto Makefile edit \"CFLAG = -DENABLE_TEST=1\"\n");
        return -1;
    }
    if(argc != 4)
    {
        printf("Usage:%s <IPAdress/Domain> <Port> <Client Number>\n", argv[0]);
        return -1;
    }
    if(isvalidip(argv[1])) /* 判断输入的是不是IP，如果不是IP，就当域名来处理 */
    {
        inet_aton(argv[1], &srvaddr.sin_addr);
    }
    else
    {
        h = gethostbyname(argv[1]); /* 有时候不起作用，或者干脆运行失败，不知道什么情况 */
        if(h==NULL)
        {
            herror("gethostbyname");
            printf(LIGHT_RED"Get domain's IP address failed,Please use IP instead of domain and restart the program!!\n"COLOR_NONE);
            return -1;
        }
        for (i = 0; (h->h_addr_list)[i] != NULL; i++)
        {
            inet_ntop(AF_INET, (h->h_addr_list)[i], ipstr, 16);
        }
        inet_aton(ipstr, &srvaddr.sin_addr);
    }
    memset(&cli_info, 0, sizeof(struct chat_info));

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(atoi(argv[2]));
    cliaddr.sin_addr.s_addr = INADDR_ANY;
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = 0;

    for(i=0; i<FD_SETSIZE; i++)
    {
        clifd[i] = -1;
    }

    n = atoi((argv[3]));
    for(i=0; i<n; i++)
    {
        sockfd = Socket(AF_INET, SOCK_STREAM, 0); //不同的连接
        clifd[i] = sockfd;
        Bind(sockfd, (SA*)&cliaddr, sizeof(cliaddr));
        Connect(sockfd, (SA*)&srvaddr, sizeof(srvaddr));
        printf("%d connect success\n", i+1);
    }
    FD_ZERO(&sel_rdset);

    for(;;)
    {
        for(i=0; i<n; i++)
        {
            FD_SET(clifd[i], &sel_rdset);
        }

        nready = Select(clifd[n-1], &sel_rdset, NULL, NULL, NULL);
        printf("nready = %d\n", nready);
        for(i=0; i<n; i++)
        {
            if(FD_ISSET(clifd[i], &sel_rdset)) /* 表示服务端有内容 */
            {
                if( Read(clifd[i], &cli_info, sizeof(struct chat_info)) == 0)
                {
                    printf(LIGHT_RED"server terminted prematurely\n"COLOR_NONE);
                    return;
                }

            }

        }
        if(cli_info.flag == SENDMSG) //如果不是群组消息就不打印了
        {
            printf(BROWN"%s\n"COLOR_NONE, cli_info.RealTime);
            printf(LIGHT_PURPLE"%s:\n"COLOR_NONE, cli_info.UserName);
            printf(LIGHT_CYAN"%s\n"COLOR_NONE, cli_info.msg);
        }
        memset(&cli_info, 0, sizeof(struct chat_info));
    }
    return 0;
}
