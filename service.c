#include<stdio.h>  
#include <sys/stat.h>
#include <unistd.h>
#include<stdlib.h>  
#include<string.h>  
#include<errno.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>
#include <pthread.h>

#define DEFAULT_PORT 8000  
#define MAXLINE 4096  


void*  connect_thread(void* arg) ;

int main(int argc, char** argv)  
{  
    int    socket_fd, connect_fd[2],n = 0;  
    struct sockaddr_in     servaddr;  
    //初始化Socket  
    if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  
    printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
    exit(0);  
    }  
    //初始化  
    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。  
    servaddr.sin_port = htons(DEFAULT_PORT);//设置的端口为DEFAULT_PORT  
  
    //将本地地址绑定到所创建的套接字上  
    if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){  
    printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
    exit(0);  
    }  
    //listen  
    if(listen(socket_fd, 10) == -1){  
    printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
    exit(0);  
    }  
    printf("======waiting for client's request======\n");   
	while(1){
	//accept  
    if( (connect_fd[n] = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1){  
        printf("accept socket error: %s(errno: %d)",strerror(errno),errno);  
        continue;  
    } 
	//create pthread
	pthread_t  connect_threadfp ;
	pthread_create(&connect_threadfp,  NULL,  connect_thread,  (void*)connect_fd[n]);
    }    
	
	close(connect_fd[n]);
    close(socket_fd);  
	return 0 ;
}  
 //thread 
 void*  connect_thread(void* arg) 
 {
	char    buff[4096];   
	int fd = (int)arg ;
	int     n; 
	//
	while(1){
	//receive date from client 
    n = recv(fd, buff, MAXLINE, 0);  
	
    //send to client 
    //if(!fork()){ 
    if(send(fd, "Hello,you are connected!\n", 26,0) == -1)  
        perror("send error");    
    //}  
    buff[n] = '\0';  
    printf("recv msg from client: %s\n", buff);
	}
	//exit(0) ;
 }
 
 
 
 