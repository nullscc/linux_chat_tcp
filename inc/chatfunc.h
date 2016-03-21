#ifndef _CHATFUNC_H_
#define _CHATFUNC_H_
#include "chatfunc.h"
#include "chat.h"

void reg_to_passwd_file(struct chat_info *info, char *filename, int sockfd);

int has_logined(int *login_flag, struct chat_info *info, struct user_info *uinfo, int maxi);

void handle_login(struct chat_info *info, char *filename, int *login_flag, int fdindex, int sockfd, struct user_info *uinfo, int maxi);

void str_echo(int listenfd);

void strcli_select(FILE* fp, int fd, struct chat_info *msginfo);

int file_exists(char *filename);

void gettime_logformat(char *buf);

void gettime_hourminsec(char *buf);

void gettime_date(char *buf);

int myfprintf(FILE *stream, const char *format, va_list *arg);

void printf_to_logfile(const char *format, ...);

void printf_to_chatlog_file(const char *format, ...);

void strcli_select(FILE* fp, int fd, struct chat_info *msginfo);

void str_echo(int listenfd);

int file_exists(char *filename);

void printf_to_logfile(const char *format, ...);

void printf_to_chatlog_file(const char *format, ...);

void gettime_hourminsec(char *buf);

#endif
