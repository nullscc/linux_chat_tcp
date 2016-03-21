#include "chatfunc.h"
#include <time.h>
#include "wrapio.h"
#include <stdio.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/***********************************
 * 功能：服务端将用户名和密码写入配置文件：/etc/chat/passwd
 * 注意点：
 *  open函数的O_CREATE必须要第3个参数存在才能起作用
 *  Readline函数需要注意保证读完一行，而不会被某种信号中断（中断时需要重新读剩下的字节）
 ***********************************/
void reg_to_passwd_file(struct chat_info *info, char *filename, int sockfd)
{
    int passwd_fd;
    char buf[100];
    char bufname[50];
    char registerresult;
    DEBUG_LONG("excute reg_to_passwd_file\n");

    passwd_fd = open(filename, O_CREAT|O_APPEND|O_RDWR, S_IRWXU);
    if(passwd_fd < 0)
    {
        perror("open or create /etc/chat/passwd failed");
        exit(1);
    }

    while( Readline(passwd_fd, buf, sizeof(buf)) != 0 )
    {
        int i;
        DEBUG("buf is %s\n", buf);
        for(i=0; i<100; i++)
        {
            if(buf[i] == ':')
            {
                DEBUG("buf[%d] = :\n", i);
                memcpy(bufname, buf, i);
                DEBUG("bufname is :%s\n", bufname);
                DEBUG("info->UserName is :%s\n", info->UserName);
                if( !strncmp(bufname, info->UserName, i) )
                {
                    registerresult = 'M';
                    Writen(sockfd, &registerresult, 1);
                    close(passwd_fd);
                    return;
                }
                break;
            }
        }
    }
    PRINTF_DESTINATION();
    if(lseek(passwd_fd, 0, SEEK_END) < 0)
    {
        perror("lseek failed");
        exit(1);
    }
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(info->UserName) + sizeof(info->UserPasswd) + 1, "%s:%s\n", info->UserName, info->UserPasswd);
    Writen(passwd_fd, buf, strlen(buf));
    registerresult = 'Y';
    Writen(sockfd, &registerresult, 1);
    close(passwd_fd);
}

/***********************************
 *功能：服务端判断某个用户是否登录
 *说明：login_flag(用户是否登录标志)与uinfo的数组下标是一一对应的
 *     info是从客户端发过来的
 *********************************/
int has_logined(int *login_flag, struct chat_info *info, struct user_info *uinfo, int maxi)
{
    int i;
    for(i=1; i<=maxi; i++)
    {
        if(!login_flag[i])
            continue;
        else
        {
            if( !strncmp(info->UserName, uinfo[i].cliname, strlen(info->UserName)) )
                return 1;
        }
    }
    return 0;
}


/*****************************
 * 功能：服务端处理客户端发送过来的登录封包：
 *      服务端收到一个登录封包，读取/etc/chat/passwd，然后比较文件中的用户名和封包UserName字段，
 *      进行登录相应处理，并给客户端反馈回一个字母，具体：
 *      R：重复登录
 *      Y：登录成功
 *      N：登录失败
 ****************************/
void handle_login(struct chat_info *info, char *filename, int *login_flag, int fdindex, int sockfd, struct user_info *uinfo, int maxi)
{
    int passwd_fd;
    char buf[100];
    char bufname[50];
    char bufpasswd[50];
    char loginresult;

    DEBUG_LONG("excute handle_login\n");
    passwd_fd = open(filename, O_CREAT|O_RDONLY, S_IRWXU);
    if(passwd_fd < 0)
    {
        perror("open /etc/chat/passwd failed");
        exit(1);
    }

    memset(bufname, 0, sizeof(bufname));
    memset(bufpasswd, 0, sizeof(bufpasswd));
    memset(buf, 0, sizeof(buf));
    while( Readline(passwd_fd, buf, sizeof(buf)) != 0 )
    {
        int i;
        DEBUG("buf is %s\n", buf);
        for(i=0; i<100; i++)
        {
            if(buf[i] == ':')
            {
                DEBUG("buf[%d] = :\n", i);
                memcpy(bufname, buf, i);
                memcpy(bufpasswd, &buf[i+1], sizeof(buf) - i - 1);
                DEBUG("bufname is :%s\n", bufname);
                DEBUG("info->UserName is :%s\n", info->UserName);

                if( !strncmp(bufname, info->UserName, strlen(info->UserPasswd)) )
                {
                    if(!strncmp(bufpasswd, info->UserPasswd, strlen(info->UserPasswd)))//strlen(bufpasswd) - i - 1))
                    {
                        DEBUG("login success\n");
                        if(has_logined(login_flag, info, uinfo, maxi))
                        {
                            loginresult = 'R';
                        }
                        else
                        {
                            login_flag[fdindex] = TRUE;
                            loginresult = 'Y';
                        }
                        Writen(sockfd, &loginresult, 1);
                        close(passwd_fd);
                        return;
                    }
                }
                break;
            }
        }


    }
    if(!login_flag[fdindex])
    {
        loginresult = 'N';
        Writen(sockfd, &loginresult, 1);
        DEBUG("login fail\n");
    }
    DEBUG("exit handle_login\n");
    close(passwd_fd);
}

/**********************************
 * 功能：服务端的处理函数，接受新连接的到来、将服务端日志与消息日志写入服务器端的文件、
 * 收取客户端的封包进行处理并根据不同的情况反馈一个封包或某些字符给客户端
 * 需要注意的：
 *  Read函数的返回值的处理
 *  需要特别声明一下的是：read返回0，表示收到一个End Of File
 *********************************/
void str_echo(int listenfd)
{
    int maxi, maxfd, nready, i, n, connfd;
    /***************
     *maxi，最大的数组下标，不减少只增加
     *maxfd，select查询的最大的描述符
     *nready，select返回的可用的可用的描述符的个数
     *connfd，accept返回的已连接的socket描述符
     **************/
    int cliselfd[FD_SETSIZE]; /* 保存客户的描述符，初始化为-1，客户断开连接了也重新置为-1 */
    fd_set rdset;
    int login_ok[FD_SETSIZE]; /* 保存的是：是否已经登录过的标志，一般与cli_record配合使用，两个数组的下标是一致的，本来可以设计到一起的，当时应该是脑袋秀逗了，分开设计了 */
    struct user_info cli_record[FD_SETSIZE]; /* 保存客户端的已登录IP等信息和用户名，方便服务端日志使用 */
    struct chat_info cli_info; /* 临时保存客户端发过来的封包，使用过后每次都需要清空，不然会有上次的字符残留 */

    memset(&cli_info, 0, sizeof(struct chat_info));
    for(i=0; i<FD_SETSIZE; i++)
    {
        cliselfd[i] = -1;
        login_ok[i] = FALSE;
    }
    cliselfd[0] = listenfd;
    maxfd = listenfd;
    maxi = 0;
    FD_ZERO(&rdset);
    for(;;)
    {
        for(i=0; i<=maxi; i++)
        {
            if( (cliselfd[i] != -1) )
            {
                FD_SET(cliselfd[i], &rdset); /* 因为select每次调用都会清空，需要重新设置 */
            }
        }

        nready = Select(maxfd+1, &rdset, NULL, NULL, NULL); /* 只监控描述符可读的情况 */
        for(i=0; i <= maxi; i++)
        {
            if(nready == 0)
            {
                break;
            }

            if(FD_ISSET(cliselfd[0], &rdset)) /* 表明有新连接过来了，需要处理 */
            {
                socklen_t len;
                len = sizeof(struct sockaddr_in);
                for(i=0; i<FD_SETSIZE; i++)
                {
                    if(cliselfd[i] == -1)
                    {
                        if(i>maxi)
                            maxi=i;
                        break;
                    }
                }
                connfd = Accept(listenfd, (SA *)&cli_record[i].cliaddr, &len);

                printf_to_logfile("%s:%d connected\n", inet_ntoa(cli_record[i].cliaddr.sin_addr), ntohs(cli_record[i].cliaddr.sin_port));
                /*************************
                 * inet_ntoa的头文件没加会报如下警告，误导程序员:
                 * chatfunc.c:229:17: warning: format ‘%s’ expects argument of type ‘char *’, but argument 2 has type ‘int’ [-Wformat=]
                 * ***********************/
                nready--;
                if(connfd > maxfd)
                    maxfd = connfd;
                cliselfd[i] = connfd;

            }
            if(FD_ISSET(cliselfd[i], &rdset)) /* 说明客户端有数据过来了，需要处理 */
            {
                nready--;

                if( (n = Read(cliselfd[i], &cli_info, sizeof(struct chat_info))) <= 0 ) /* 需要处理read返回小于0的情况，这很重要 */
                {
                    if( (n < 0) && (errno == ECONNRESET) )
                    {
                        printf_to_logfile("User:%s IP:%s:%d has aborted the connection\n", cli_record[i].cliname, inet_ntoa(cli_record[i].cliaddr.sin_addr), ntohs(cli_record[i].cliaddr.sin_port));
                        FD_CLR(cliselfd[i], &rdset);
                        close(cliselfd[i]);
                        cliselfd[i] = -1;
                        login_ok[i] = -1;
                        memset(&cli_record[i], 0, sizeof(struct user_info));
                        continue;
                    }
                    else if(n < 0)
                        exit(1);
                    if(n == 0)
                    {
                        printf_to_logfile("User:%s IP:%s:%d has terminted the connection\n", cli_record[i].cliname, inet_ntoa(cli_record[i].cliaddr.sin_addr), ntohs(cli_record[i].cliaddr.sin_port));
                        FD_CLR(cliselfd[i], &rdset);
                        close(cliselfd[i]);
                        cliselfd[i] = -1;
                        login_ok[i] = -1;
                        memset(&cli_record[i], 0, sizeof(struct user_info));
                        continue;
                    }
                }

                if(cli_info.flag == REGISTER) /* 处理客户端发过来的注册封包处理 */
                {
                    reg_to_passwd_file(&cli_info, "/etc/chat/passwd", cliselfd[i]);
                    printf_to_logfile("IP:%s:%d Register\n", inet_ntoa(cli_record[i].cliaddr.sin_addr), ntohs(cli_record[i].cliaddr.sin_port));
                }
                else if(cli_info.flag == LOGIN) /* 处理客户端发过来的登录封包处理 */
                {
                    handle_login(&cli_info, "/etc/chat/passwd", login_ok, i, cliselfd[i], cli_record, maxi);
                    if(login_ok[i])
                    {
                        memcpy(cli_record[i].cliname, cli_info.UserName, sizeof(cli_info.UserName));
                        printf_to_logfile("User:%s IP:%s:%d Login\n", cli_record[i].cliname, inet_ntoa(cli_record[i].cliaddr.sin_addr), ntohs(cli_record[i].cliaddr.sin_port));
                    }
                }
                else if(cli_info.flag == COMMAND) /* 处理客户端发过来的登录封包处理 */
                {
                    srv_handle_cmd(cliselfd[i], &cli_info, login_ok, maxi, cli_record);
                    printf_to_logfile("User:%s IP:%s:%d Send Command:%s", cli_record[i].cliname, inet_ntoa(cli_record[i].cliaddr.sin_addr), ntohs(cli_record[i].cliaddr.sin_port), cli_info.cmd);
                    memset(&cli_info, 0, sizeof(struct chat_info));
                }
                else if(cli_info.flag == PRIVATEMSG) /* 处理客户端发过来的私聊封包处理 */
                {
                    srv_handle_prv_chat(i, cliselfd, &cli_info, login_ok, maxi, cli_record);
                    printf_to_logfile("User:%s IP:%s:%d Private To:%s:%s\n", cli_record[i].cliname, inet_ntoa(cli_record[i].cliaddr.sin_addr), ntohs(cli_record[i].cliaddr.sin_port), cli_info.PrvName, cli_info.msg);
                    memset(&cli_info, 0, sizeof(struct chat_info));
                }

                else if(cli_info.flag == SENDMSG)   /* 处理客户端发过来的群组封包处理 */
                {
                    gettime_hourminsec(cli_info.RealTime);
                    for(i=1; i<=maxi; i++)
                    {
                        if(cliselfd[i] != -1)
                        {
                            #if !ENABLE_TEST //如果是测试的情况下，不是已登录的用户也要发送
                            if(!login_ok[i])
                                continue;
                            #endif

                            Writen(cliselfd[i], &cli_info, sizeof(struct chat_info) - (MAXLINE-strlen(cli_info.msg)));
                        }
                    }
                    printf_to_chatlog_file("%s\n%s:%s\n", cli_info.RealTime, cli_info.UserName, cli_info.msg);
                    printf_to_logfile("User:%s IP:%s:%d Send Group MSG:%s", cli_record[i].cliname, inet_ntoa(cli_record[i].cliaddr.sin_addr), ntohs(cli_record[i].cliaddr.sin_port), cli_info.msg);
                    memset(&cli_info, 0, sizeof(struct chat_info));
                }
                }
        }
    }
}

/*********************************************
 *功能：客户端处理函数：接受用户输入、接收服务端发过来的封包并显示到标准输出
 *需要注意的是：如果用户输入反馈到结尾了，需使用shutdown而不是close，因为还要处理服务端发过来的封包
 *            但是其实这里是不存在这个问题的，因为一般结束用户程序一般都是直接结束了
 *********************************************/
void strcli_select(FILE* fp, int fd, struct chat_info *msginfo)
{
    char buf[MAXLINE]; /* 临时存储用户输入的字符串 */
    struct chat_info rcvinfo; /* 临时存储收到的服务端的封包，以便处理 */
    fd_set sel_rdset; /* 供select使用 */
    int maxfd, n; /* maxfd为最大的监控描述符 */
    int stdineof = 0; /* 用户输入是否结束，如果用户输入没有结束，但是收到sock连接的EOF表示服务器异常终止了*/

    FD_ZERO(&sel_rdset);
    for(;;)
    {
        FD_SET(fileno(fp), &sel_rdset);
        FD_SET(fd, &sel_rdset);
        maxfd = max(fileno(fp), fd) + 1;
        DEBUG("wait for data\n");
        Select(maxfd, &sel_rdset, NULL, NULL, NULL);
        DEBUG("select return\n");
        if(FD_ISSET(fd, &sel_rdset)) /* 表示服务端有内容 */
        {
            if( Read(fd, &rcvinfo, sizeof(struct chat_info)) == 0)
            {
                if(stdineof == 0)
                {
                    printf(LIGHT_RED"server terminted prematurely\n"COLOR_NONE);
                }
                return;
            }
            DEBUG("sockfd in %s %d has data\n", __FILE__, __LINE__);
            printf(BROWN"%s\n"COLOR_NONE, rcvinfo.RealTime);
            if(rcvinfo.flag == PRIVATEMSG)
            {
                char uname[25] = {0};
                if( !strncmp(rcvinfo.PrvName, uname, 25) )
                {
                    printf(LIGHT_RED"%s\n"COLOR_NONE, rcvinfo.msg);
                    memset(&rcvinfo, 0, sizeof(struct chat_info));
                    continue;
                }
                printf(LIGHT_GREEN"@ from "COLOR_NONE);
                printf(LIGHT_PURPLE"%s:\n"COLOR_NONE, rcvinfo.UserName);

            }
            else
                printf(LIGHT_PURPLE"%s:\n"COLOR_NONE, rcvinfo.UserName);
            printf(LIGHT_CYAN"%s\n"COLOR_NONE, rcvinfo.msg);

            memset(&rcvinfo, 0, sizeof(struct chat_info));

        }
        if(FD_ISSET(fileno(fp), &sel_rdset)) /* 表示从标准输入有数据过来，即用户输入 */
        {
            if( (n = Read(fileno(fp), buf, MAXLINE)) == 0)
            {
                    stdineof = 1;
                    shutdown(fd, SHUT_WR);
                    FD_CLR(fileno(fp), &sel_rdset);
                    continue;
            }

            if( (buf[0] == '\n')  ) /* 如果用户输入单纯的是一个回车符，用于终端处理的数据只有一行，所有不予理会 */
                continue;
            if(buf[0] == ':') /* 首字节为':',表示是一个命令，不是一个普通的群组消息封包 */
            {
                if(buf[1] == '@')
                {
                    msginfo->flag = PRIVATEMSG; /* ':'后面跟'@'表示是一个私聊消息 */
                    if( (buf[2]) == ' ' || (buf[2]) == '\n' )  /* 如果'@'紧跟的是空格或者回车，则不认为它是一个有效的命令 */
                    {
                        printf(LIGHT_RED"Error Instruction!!!\n"COLOR_NONE);
                        continue;
                    }
                    get_prvname(msginfo->PrvName, &buf[2]);
                    get_prvmsg(msginfo->msg, &buf[2]);
                    Writen(fd, msginfo, sizeof(struct chat_info) - (MAXLINE-strlen(msginfo->msg)));
                    memset(buf, 0, MAXLINE);
                    memset(msginfo->msg, 0, MAXLINE);
                    memset(msginfo->PrvName, 0, sizeof(msginfo->PrvName));
                }
                else
                {
                    msginfo->flag = COMMAND;
                    memcpy(msginfo->cmd, &buf[1], strlen(&buf[1]));
                    send_cmd_to_srv(fd, msginfo);
                    recieve_cmd_result_from_srv(fd, msginfo);
                    memset(buf, 0, MAXLINE);
                    memset(msginfo->cmd, 0, sizeof(msginfo->cmd));
                }
                continue;

            }
            else
            {
                msginfo->flag = SENDMSG;
            }
            memset(msginfo->msg, 0, MAXLINE);
            memcpy(msginfo->msg, buf, strlen(buf));
            DEBUG("stdin in %s has data\n", __FILE__);
            Writen(fd, msginfo, sizeof(struct chat_info) - (MAXLINE-strlen(buf)));
            memset(buf, 0, MAXLINE);
            memset(msginfo->msg, 0, MAXLINE);
        }

    }

}

/*******************************
 * 功能：判断某个文件是否存在，filename参数需要绝对路径名
 ******************************/
int file_exists(char *filename)
{
    return (access(filename, 0) == 0);
}

/*********************************
 * 功能：将服务器当前的时间以"年月日小时分秒"数字的形式存入buf指向的内存中
 ********************************/
void gettime_logformat(char *buf)
{
    struct tm *tm_t;
    int year, month, day, hour, min, second;
    time_t ticks;
    ticks = time(NULL);
    tm_t = localtime(&ticks);
    year  = 1900 + tm_t->tm_year;
    month = tm_t->tm_mon + 1;
    day   = tm_t->tm_mday;
    hour  = tm_t->tm_hour;
    min   = tm_t->tm_min;
    second= tm_t->tm_sec;
    snprintf(buf, 15, "%d%02d%02d%02d%02d%02d", year, month, day, hour, min, second);

}

/*********************************
 * 功能：将服务器当前的时间以"小时:分钟:秒"的形式存入buf指向的内存中
 ********************************/
void gettime_hourminsec(char *buf)
{
    struct tm *tm_t;
    int hour, min, sec;
    time_t ticks;

    ticks = time(NULL); /* time() 函数返回自 Unix 纪元（January 1 1970 00:00:00 GMT）起的当前时间的秒数。 */
    tm_t = localtime(&ticks); /* 得到struct tm结构体，方便算出时间 */

    hour  = tm_t->tm_hour;
    min   = tm_t->tm_min;
    sec= tm_t->tm_sec;
    snprintf(buf, 10, "%02d:%02d:%02d", hour, min, sec);
}

/*********************************
 * 功能：将服务器当前的时间以"年月日"数字的形式存入buf指向的内存中
 ********************************/
void gettime_date(char *buf)
{
    struct tm *tm_t;
    int year, month, day;
    time_t ticks;

    ticks = time(NULL);
    tm_t = localtime(&ticks);

    year  = 1900 + tm_t->tm_year;
    month = tm_t->tm_mon + 1;
    day   = tm_t->tm_mday;

    snprintf(buf, 9, "%d%02d%02d", year, month, day);

}

/**************************************
 *功能：参照glibc源码中的fprintf的实现，只不过忽略了va_list类型变量的获取过程
 ************************************/
int myfprintf (FILE *stream, const char *format, va_list *arg)
{

  int done;
  done = vfprintf (stream, format, *arg);
  return done;
}

/**************************************
 *功能：将客户端的连接、登录、注册、私聊、群聊过程记录到服务器中
 ************************************/
void printf_to_logfile(const char *format, ...)
{
    char buf[15];
    char date[9];
    char datefilename[30] = SRVLOGDIR;
    FILE *fp;
    va_list arg;
    va_start (arg, format);

    memset(buf, 0, sizeof(buf));
    memset(date, 0, sizeof(date));

    if(!is_dir_exist(SRVLOGDIR))
    {
        if(mkdir(SRVLOGDIR, S_IRWXU) < 0)
        {
            perror("mkdir error");
            return;
        }
    }

    gettime_date(date);
    strcat(datefilename, date);
    fp = fopen(datefilename, "a+");
    if(fp == NULL)
    {
        perror("fopen error");
        return;
    }

    gettime_logformat(buf);
    fprintf(fp, "%s:  ", buf);
    //fflush(stdout);
    //printf(format， arg); //用这个参数传不进来
    //fprintf(fp, format, arg); //必须用这个vfprintf(stdout, format, arg)
    myfprintf(fp, format, &arg); //arg不能穿越函数

    va_end(arg);

    fclose(fp);
}

/**************************************
 *功能：将客户端的群聊过程记录到服务器中，方便做离线消息，考虑到目前是单进程多路复用的情况，就没做离线消息了，但是还是在服务端保存离线消息
 ************************************/
void printf_to_chatlog_file(const char *format, ...)
{
    char buf[15];
    char date[9];
    char datefilename[30] = CHATLOGDIR;
    FILE *fp;

    va_list arg;
    va_start (arg, format);

    memset(buf, 0, sizeof(buf));
    memset(date, 0, sizeof(date));

    if(!is_dir_exist(datefilename))
    {
        if(mkdir(datefilename, S_IRWXU) < 0)
        {
            perror("mkdir error");
            return;
        }
    }
    gettime_date(date);
    strcat(datefilename, date);

    fp = fopen(datefilename, "a+");
    if(fp == NULL)
    {
        perror("fopen error");
        return;
    }
    myfprintf(fp, format, &arg); //arg不能穿越函数
    va_end(arg);
    fclose(fp);
}
