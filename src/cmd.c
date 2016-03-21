#include "cmd.h"

/*************************************
 * 功能：服务端收到客户端发送过来的查询当前在线人数的指令，将人数以字符串的形式反馈回去
 ************************************/
void write_online_num_to_cli(int fd, int *login_ok, int maxi)
{
    int i;
    char buf[10];
    int num_ret;
    num_ret = 0;
    for(i=1; i<=maxi; i++)
    {
        if(login_ok[i])
            num_ret++;
    }
    snprintf(buf, 10, "%d", num_ret);
    Writen(fd, buf, strlen(buf));
}

/*************************************
 * 功能：服务端收到客户端发送过来的查询当前在线名单的指令，将名单以字符串的形式反馈回去
 ************************************/
void write_online_name_to_cli(int *login_ok, struct user_info *info, int maxi, int fd)
{
    int i;
    char buf[FD_SETSIZE*25];
    int index = 0;
    int n;
    memset(buf, 0, sizeof(buf));//如果不加这句，会有有趣的事情出现
    for(i=1; i<=maxi; i++)
    {
        if(login_ok[i])
        {
            n = strlen(info[i].cliname)+1;
            snprintf(&buf[index], n, "%s", info[i].cliname);
            buf[index+n-1] = '\n';
            index += n;
        }
    }

    Writen(fd, buf, strlen(buf));
}

/*************************************
 * 功能：服务端收到客户端发送过来的帮助指令，将/etc/chat/help发送给客户端
 ************************************/
void write_help_info_to_cli(int fd)
{
    char buf[FD_SETSIZE*25];
    int helpfd;
    memset(buf, 0, sizeof(buf));
    helpfd = open("/etc/chat/help", O_RDONLY);
    Read(helpfd, buf, FD_SETSIZE*25);
    Writen(fd, buf, strlen(buf));
}

/*************************************
 * 功能：服务端处理客户发过来的指令封包
 ************************************/
void srv_handle_cmd(int fd, struct chat_info *info, int *login_ok, int maxi, struct user_info *uinfo)
{
    DEBUG("cmd is:%s\n", info->cmd);
    char unsupport[] = "unsupport instuction";
    DEBUG("excute srv_handle_cmd\n");
    if( !strncmp(info->cmd, "onlinenum\n", strlen(info->cmd)) )
    {
        write_online_num_to_cli(fd, login_ok, maxi);
    }
    else if( !strncmp(info->cmd, "onlinename\n", strlen(info->cmd)) )
    {
        write_online_name_to_cli(login_ok, uinfo, maxi, fd);
    }
    else if( !strncmp(info->cmd, "help\n", strlen(info->cmd)) )
    {
        write_help_info_to_cli(fd);
    }
    else
        Writen(fd, unsupport, sizeof(unsupport));

}

/*************************************
 * 功能：客户端发送指令封包给服务端
 ************************************/
void send_cmd_to_srv(int fd, struct chat_info *msginfo)
{
    Writen(fd, msginfo, sizeof(struct chat_info));
}

/*************************************
 * 功能：客户端接收服务端对客户端发过去的指令封包的反馈，并打印结果
 ************************************/
void recieve_cmd_result_from_srv(int fd, struct chat_info *msginfo)
{
    char buf[FD_SETSIZE*25];
    //int n;
    DEBUG("excute recieve_cmd_result_from_srv\n");
    memset(buf, 0, sizeof(buf));
    if( !strncmp(msginfo->cmd, "onlinenum\n", strlen(msginfo->cmd)) )
    {
        Read(fd, buf, sizeof(buf));
        printf(YELLOW"%s online people\n"COLOR_NONE, buf);
    }
    else if( !strncmp(msginfo->cmd, "onlinename\n", strlen(msginfo->cmd)) )
    {
        Read(fd, buf, sizeof(buf));
        printf(YELLOW"online people:\n%s\n"COLOR_NONE, buf);
    }
    else if( !strncmp(msginfo->cmd, "help\n", strlen(msginfo->cmd)) )
    {
        Read(fd, buf, sizeof(buf));
        printf("-------------------------------------------------------------------\n");
        printf(LIGHT_CYAN"\nBelow Is Help Info.\n\n"COLOR_NONE);
        printf(YELLOW"%s\n"COLOR_NONE, buf);
        printf("-------------------------------------------------------------------\n");
    }
    else
    {
        Read(fd, buf, sizeof(buf));
        printf(LIGHT_RED"%s\n"COLOR_NONE, buf);
    }
}

/*************************************
 * 功能：客户端从形参buf中提取Username字段，并存入prvname
 ************************************/
void get_prvname(char *prvname,char *buf)
{
    memset(prvname, 0, 25);
    int i;
    for(i=0; i<25; i++)
    {
        if(buf[i] == ' ')
            break;
    }
    memcpy(prvname, buf, i);
}

/*************************************
 * 功能：客户端从形参buf中提取msg字段，并存入prvmsg
 ************************************/
void get_prvmsg(char *prvmsg, char *buf)
{
    memset(prvmsg, 0, MAXLINE);
    int i;
    for(i=0; i<MAXLINE; i++)
    {
        if(buf[i] == ' ')
            break;
    }
    memcpy(prvmsg, &buf[i+1], strlen(&buf[i+1]));
}

/*************************************
 * 功能：服务器端处理客户端发送过来的私聊封包
 ************************************/
void srv_handle_prv_chat(int sendinex, int *clifd, struct chat_info *info, int *login_ok, int maxi, struct user_info *uinfo)
{
    int i;
    char errmsg[] = "Your @user is not online or not exist!!!";
    for(i=1;i<=maxi;i++)
    {
        if(login_ok[i])
        {
            if( !strncmp(info->PrvName, uinfo[i].cliname, strlen(info->PrvName)) )
            {
                gettime_hourminsec(info->RealTime);
                Writen(clifd[i], info, sizeof(struct chat_info) - (MAXLINE-strlen(info->msg)));
                memcpy(info->msg, errmsg, sizeof(errmsg));
                return;
            }
        }
    }

    memcpy(info->msg, errmsg, sizeof(errmsg));
    Writen(clifd[sendinex], info, sizeof(struct chat_info) - (MAXLINE-strlen(info->msg)));
}
