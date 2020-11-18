#include "do_passwd.h"


static char passwd_save[6] = {0x31,0x32,0x33,0x34,0x35,0x36} ;
static char passwd_buf[6] = {0} ;


static int passwd_fd = 0 ;

int passwd_open() 
{
	passwd_fd = open("/root/password.txt",O_RDWR | O_CREAT,S_IRUSR | S_IWUSR) ;
	if(passwd_fd < 0 ){
		printf("open password fail\n") ;
		return -1 ;
	}
	return 0 ;
}

int passwd_write(char* pdate)
{
    int ret =0 ;
	int i = 0 ;

	ret = write(passwd_fd,pdate,sizeof(passwd_save)) ;
	printf("\n password has save finish \n") ;
	lseek(passwd_fd,0,SEEK_SET); 

	return 0 ;
}

char* passwd_read()
{
	int ret = 0 ;
    int i = 0 ;

	ret = read(passwd_fd,passwd_buf,sizeof(passwd_buf)) ;
	lseek(passwd_fd,0,SEEK_SET); 

	printf("read :%d\n",ret) ;
	if(ret == 0)  //default password
	{
       passwd_buf[0] = 0x36 ;
       passwd_buf[1] = 0x36 ;
       passwd_buf[2] = 0x36 ;
       passwd_buf[3] = 0x36 ;
       passwd_buf[4] = 0x36 ;
       passwd_buf[5] = 0x36 ;
	}
	return passwd_buf ;
}
