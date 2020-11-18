#ifndef  __CMI_AT155_APPLICATION_H
#define  __CMI_AT155_APPLICATION_H

#include "post_to_service.h"
#include "do_passwd.h"
#include "set_ip_adress.h"
/*
 * socket
 */

#define   MAXLINE 4096  
#define   CONNECT_CLIENT_MAX    100

#define   DEFAULT_PORT  21 // 8000  
#define   CONNECT_IP    "192.168.4.100"

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
#include <netinet/tcp.h>   //SOL_TCP ...  for KeepAlive
#include<arpa/inet.h>

/* 
 * input signal 
 */
//#include <stdio.h>
//#include <sys/stat.h>
#include <sys/fcntl.h>
//#include <sys/types.h>
//#include <string.h>
//#include <unistd.h>
#include <sys/select.h>
#include <signal.h>

/* 
 * system time 
 * */
#include<time.h>  //getSystemTimer
#include<sys/time.h>
#include <sys/timeb.h>

/* 
 * uart
 */
#include <termios.h>


/*
 *ioctl
 */
#include <sys/ioctl.h>

/*
 *     ioctl  MV1--(1,0)  SPS--(1,1)  SPE--(0,1)   MV3--(1,1)
 *            start       palse       stop_palse   stop
 */
#define      STATUS_MV1		_IO('l',  0)
#define      STATUS_SPS		_IO('l',  1)
#define      STATUS_SPE		_IOW('l',  2,  long)
#define      STATUS_MV3		_IOW('l',  3,  long)
#define      BEEP_ON		_IOW('l',  4,  long)
#define      BEEP_OFF		_IOW('l',  5,  long)
#define      DISCONNECT		_IOW('l',  6,  long)
#define      ON_CONNECT		_IOW('l',  7,  long)

enum  POST_EVENT{
	MV1 = 1 ,     //start
	MV2 ,         //post   right now
	SPS,          //palse  to  post
	SPE,          //stop   pulse  to run
	MV3,          //stop   action
	//_ALARM_,      //alarm  to  post
};

enum  IPSETSTATUS{
	NOSET = 0,
	SETOK ,
	SETFINISH ,
};

enum  eth0_flag{
	eth0_down,
	eth0_up,
} ;
void show_job_number(char* job_number) ;
void NoTask(void) ;

#endif
