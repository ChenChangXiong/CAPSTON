#include "cmi_at155_application.h"

static char  post_buf[98] = {0} ;
static char  post_id_buf[13] = "000000000001\n" ;
static char  post_time_buf[20] = "2019-05-20 13:00:00\n" ;
static char  post_eventcode_buf[11] = "xxxxxxxMV1\n" ;
static char  post_code[31] = "5120010001101201905000001xxxxx\n" ; 
static char  post_Qty[15] = "00000000000000\n" ;
static char  post_errorcdode_buf[8] = "00000003" ;
static char  num[10] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39} ;

pthread_t  connect_threadfp ; 
int        sockfd = 0 ;
struct     sockaddr_in    servaddr; 
void*      connect_thread(void* arg) ;

enum  client_socket_status{ 
	disconnect = 0,
	connected ,
};

static  char* connect_ip_string ;

static enum client_socket_status  connect_status = disconnect ;

void    socket_client_init()
{
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){  
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);  
		exit(0);  
	}  

	memset(&servaddr, 0, sizeof(servaddr));  
	servaddr.sin_family = AF_INET;  
	servaddr.sin_port = htons(8000);  
	if(inet_pton(AF_INET, connect_ip_string, &servaddr.sin_addr) <= 0){  
		printf("inet_pton error for %s\n",CONNECT_IP);  
		exit(0);  
	}  
}
void    re_connect() 
{
	int ret = 0  ;
	
	socket_client_init() ; //re init socket
	if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){  
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);  
		close(sockfd);  
	}else{
		printf("---reconnect finish---\n") ;
		ret = pthread_create(&connect_threadfp,  NULL,  connect_thread,  (void*)sockfd);
		connect_status = connected ;             //reconnect successed
	}
    sleep(3) ;
}
int main(int argc, char** argv)  
{  
	int     ret = 0 ;
	int     rec_len = 0 ;  
	char    buf[MAXLINE];  
    /*
	if( argc != 2){  
		printf("usage: ./client <ipaddress>\n");  
		exit(0);  
	}  
    */
	if(argc==2){
		connect_ip_string = argv[1] ;
	}else{
		connect_ip_string = CONNECT_IP ;
	}
	socket_client_init() ;
    //connect
	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){  
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);  
		exit(0);  
	}  
	printf("--connect succes--\n") ;
	connect_status = connected ;
	//creat thread for receive
	ret = pthread_create(&connect_threadfp,  NULL,  connect_thread,  (void*)sockfd);
	//pipe  broken
	//signal(SIGPIPE, SIG_IGN);//忽略管道破裂信号
	while(1){

		memcpy(post_buf,post_id_buf,sizeof(post_id_buf)) ;
		memcpy(&post_buf[13],post_time_buf,sizeof(post_time_buf)) ;
		memcpy(&post_buf[13 + 20],post_eventcode_buf,sizeof(post_eventcode_buf)) ;
		memcpy(&post_buf[13 + 20 + 11],post_code,sizeof(post_code)) ;
		memcpy(&post_buf[13 + 20 + 11 + 31],post_Qty,sizeof(post_Qty)) ;
		memcpy(&post_buf[13 + 20 + 11 + 31 + 15],post_errorcdode_buf,sizeof(post_errorcdode_buf)) ;

		if(connect_status == connected){
			ret = send(sockfd,post_buf,sizeof(post_buf),0) ;
			if(ret  < 0)  
			{  
				printf("send msg error: %s(errno: %d)\n", strerror(errno), errno); 
				connect_status = disconnect ;
				close(sockfd);  
			}

			printf("---%d---\n",ret) ;
			sleep(2) ;
		}else{                             //connect has been broken  , reconnect right now

				re_connect() ;
		}
	}
	close(sockfd);  
	return 0 ;  
} 

//thread 
void*  connect_thread(void* arg) 
{
	char    buff[100];   
	int     n; 
	
	while(1){

		if(connect_status == connected){
			n = recv(sockfd, buff, MAXLINE, 0);  //receive date  

			if(n>0)
			{
				buff[n] = '\0';  
				printf("\nservice send date : %s\n", buff);
			}
			else
			{
				perror("recv error");  
				connect_status = disconnect ;
				close(sockfd);  
				//break;     //结束线程  
			}
		}
	}
		printf("--thread close now--\n") ;
}
