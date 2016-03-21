#include "chat.h"
#include <netdb.h>


/**************************
 *功能：客户端运行起始处,程序运行方法：
 *  clichat <IP地址/域名> <端口>
 *************************/
int main(int argc, char**argv)
{
	int sockfd;
	struct sockaddr_in srvaddr;
	struct chat_info cli_info;
	char option;
    int n, i;
    char result[10];
    struct hostent *h;
    char ipstr[16];
    if(argc != 3)
	{
        printf("Usage:%s <IPAdress/Domain> <Port>\n", argv[0]);
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

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(atoi(argv[2]));

    Connect(sockfd, (SA*)&srvaddr, sizeof(srvaddr));

    printf(LIGHT_PURPLE"-------- Welcome Linux Terminal Chat Room --------\n"COLOR_NONE);
    printf(LIGHT_CYAN"Please Select Option Below:\n"COLOR_NONE);
FIRST_IN:
    printf(LIGHT_CYAN"--------------------\n"COLOR_NONE);
    printf(LIGHT_CYAN"| 1:Register       |\n"COLOR_NONE);
    printf(LIGHT_CYAN"| 2:Login          |\n"COLOR_NONE);
    printf(LIGHT_CYAN"--------------------\n"COLOR_NONE);
    //read(fileno(stdin), &option, 1);
    option = getc(stdin);

    if(option == '1')
    {
        cli_info.flag = REGISTER;
        printf_flush(LIGHT_CYAN"Please Input An User Name:"COLOR_NONE);
        n = Sockread(fileno(stdin), cli_info.UserName, sizeof(cli_info.UserName));
        cli_info.UserName[n-1] = '\0'; //取消输入的'\n'

        printf_flush(LIGHT_CYAN"Please Input A passwd:"COLOR_NONE);
        n = Sockread(fileno(stdin), cli_info.UserPasswd, sizeof(cli_info.UserPasswd));
        cli_info.UserPasswd[n-1] = '\0'; //取消输入的'\n'
    }
    else if(option == '2')
    {
        cli_info.flag = LOGIN;
    }
    else
    {
        cli_info.flag = NONEOPTION;
        printf(LIGHT_RED"Wrong Option,Please Select Option Below:\n"COLOR_NONE);
        clearbuf(0);
        goto FIRST_IN;
    }

    if(cli_info.flag == REGISTER)
    {
        PRINTF_DESTINATION();
        Writen(sockfd, &cli_info, sizeof(struct chat_info) - (MAXLINE-strlen(cli_info.msg)));
        PRINTF_DESTINATION();
        //需要处理同名的情况
        n = Sockread(sockfd, result, 10);
        PRINTF_DESTINATION();
        clearbuf(1);
        PRINTF_DESTINATION();
        if(result[0] == 'M')
        {
            printf(LIGHT_RED"The User Name Has Already Be Register,Please Select Anothe Name:\n"COLOR_NONE);
        }
        if(result[0] == 'Y')
        {
            printf(LIGHT_CYAN"Register Success,Please Select Option Below:\n"COLOR_NONE);
        }
        clearbuf(0);
        goto FIRST_IN;

    }
    else if(cli_info.flag == LOGIN)
    {
        printf_flush(LIGHT_CYAN"Your Name:"COLOR_NONE);
        n = Sockread(fileno(stdin), cli_info.UserName, sizeof(cli_info.UserName));
        cli_info.UserName[n-1] = '\0'; //取消输入的'\n'

        printf_flush(LIGHT_CYAN"Your Passwd:"COLOR_NONE);
        DEBUG_LONG("Read from input n=%d\n", n);
        n = Sockread(fileno(stdin), cli_info.UserPasswd, sizeof(cli_info.UserPasswd));
        DEBUG_LONG("Read from input n=%d\n", n);
        cli_info.UserPasswd[n-1] = '\0'; //取消输入的'\n'
        n = Writen(sockfd, &cli_info, sizeof(struct chat_info) - (MAXLINE-strlen(cli_info.msg)));
        DEBUG_LONG("writen to srv n=%d\n", n);
        n = Sockread(sockfd, result, 10);
        DEBUG_LONG("Read from srv n=%d\n", n);
        clearbuf(1); /* 清除标准输入的缓冲区，不然下一次输入会残留，clearbuf(0)效果一样，只不过clearbuf(0)当缓冲区没有东西时，会阻塞*/

        if(result[0] == 'N') /* 登录结果：失败，服务端发送回来的 */
        {
            printf(LIGHT_RED"User Name or Passwd Incorrect,Please Retry Below:\n"COLOR_NONE);
            clearbuf(0);
            goto FIRST_IN;
        }
        if(result[0] == 'R')/* 登录结果：已经登录过了 */
        {
            printf(LIGHT_RED"The User Has Already Logined,Please Retry Below:\n"COLOR_NONE);
            clearbuf(0);
            goto FIRST_IN;
        }
        if(result[0] == 'Y')/* 登录结果：成功 */
        {
            printf(LIGHT_CYAN"Login Success,You Could Send Message To You Want!\n"COLOR_NONE);
        }

    }
    cli_info.flag = SENDMSG; /* 注册、登录完以后其他均视为群组消息 */
    clearbuf(1);

	strcli_select(stdin, sockfd, &cli_info);


	return 0;
}
