#ifndef   __CLIENT_FTP_H
#define   __CLIENT_FTP_H


#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<unistd.h>
#include<netinet/in.h>
#include<netdb.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <arpa/inet.h>

#define SERV_PORT 21
#define MAXSIZE 1024
#define SA struct sockaddr

#define   FTP_CONNECT_IP   "192.168.2.100"

static int control_sockfd;
int npsupport;
int login_yes;
int f;//f=0时为默认文件结构
int login();
void ftp_list(int control_sockfd);
void zeromery(char *a,int len);
void ftp_pwd(int control_sockfd, char* goto_folder);
void ftp_changdir(char dir[],int control_sockfd);
void ftp_quit(int control_sockfd);
void ftp_creat_mkd(char *path,int control_sockfd);
void ftp_back(int control_sockfd);
void ftp_stru(int control_sockfd);
void ftp_rest(int control_sockfd);
int ftp_download(int control_sockfd);
char *itoa(int value, char *string, int radix);
int ftp_up(int control_sockfd, char* localpathname, char* pathname) ;


#endif



