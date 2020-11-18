/*
 u*  include all  include.h
 *  1.tcp
 *  2.module_test
 *  3.uart
 */
#include "cmi_at155_application.h"
#include "client_ftp.h"

//#define  LCD_UART_DEBUG //触摸屏调试宏定义
//#define  TIME_DEBUG

static  int driver_misc_fd = 0 ;
unsigned  long  POST_COUNT = 0 ; 
unsigned  long  target = 50000 ; 
static  char   target_flag = 0 ;
static  int     socket_fd, connect_fd ;
static  enum    eth0_flag  eth0_connect_status = eth0_down ;
//tcp date
static  char    post_buf[100] = {0} ;
static  char    post_id_buf[13] = "000000000001 " ;
static  char    post_time_buf[20] = "2019-05-20 13:00:00 " ;
static  char    post_eventcode_buf[11] = "xxxxxxxMV1 " ;
static  char    post_code[31] = "5120010001101201906000001xxxxx " ; 
static  char    post_Qty[15] = "00000000000000 " ;
static  char    post_errorcdode_buf[8] = "00000000" ;
static  char    num[10] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39} ;

static  char    connected_flag = 0 ;
static  int     re_login = 0 ;
int     alarm_time = 180 ;           //3600
static  char    systemtime[30] ;

static  struct  termios termold,termnew;
static  int     uart_fd ;
static  char    delay_for_palse_flag = 0 ; 

enum   WINDOWS_FLAG{ PASSWD_WINDOW = 1, GET_ID_WINDOW,  MAIN_WINDOW , } ;
   //0-->passwdwindows  1-->mainwindows

enum  POST_EVENT   Event_ID = MV3 ;        //启动直接允许报工
enum  POST_EVENT   Temp_Event_ID = MV3 ;   //用于掉线重发的SPE SPS临时变量

static enum  WINDOWS_FLAG UART_WINDOWS = PASSWD_WINDOW ; 

void   input_DI_handler(int sig) ;  //fasync signal handler
void   alarm_post() ;    
void   stop_signal() ;  //CTRL + C

enum   IPSETSTATUS  IPSTATUS = NOSET ;  //设置设备ip地址
int ftp_sockfd = 0 ; 
static  char sleep_after_stop = 0 ; //按下停止 睡眠1分钟

int log_fd = 0 ;

//send buf
char send_temp[18] = {0} ;
char send_clear_txt_buf[12] = {0xEE,0xB1,0x10,0x00,0x00,0x00,0x01,0x20,0xFF,0xFC,0xFF,0xFF} ;
char send_passwd_buf[17]={0xEE,0xB1,0x10,0x00,0x00,0x00,0x01,0x30,0x31,0x32,0x33,0x32,0x35,0xFF,0xFC,0xFF,0xFF} ; //send buf  [7-12] 
char machine_id_buf[9] = {0xEE, 0xB1, 0x00, 0x00, 0x05, 0xFF,0xFC, 0xFF, 0xFF} ; //机器码输入界面
char enter_buf[9] = {0xEE, 0xB1, 0x00, 0x00, 0x01, 0xFF,0xFC, 0xFF, 0xFF} ;   //start to mainwindows
char error_buf[9] = {0xEE, 0xB1, 0x00, 0x00, 0x02, 0xFF,0xFC, 0xFF, 0xFF} ;   //show  error 
char start_UI_buf[9] = {0xEE, 0xB1, 0x00, 0x00, 0x00, 0xFF,0xFC, 0xFF, 0xFF} ;   //return back to password windows

char time_buf[17] = {0xEE,0xB1,0x52,0x00,0x01,0x00,0x06,0x30,0x39,0x3A,0x31,0x36,0x3B,0xFF,0xFC,0xFF,0xFF} ; //[7-8] [10-11]
char send_count_buf[25] = {0xEE,0xB1,0x52,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF, 0xFC, 0xFF, 0xFF} ; //[7-20]

char send_start_status[15] = {0xEE,0xB1,0x10,0x00,0x01,0x00,0x04,0xBF,0xAA,0xCA,0xBC,0xFF,0xFC,0xFF,0xFF} ;
char send_palse_status[19] = {0xEE,0xB1,0x10,0x00,0x01,0x00,0x04,0x20,0x20,0xD4,0xDD,0xCD,0xA3,0x20,0x20,0xFF,0xFC,0xFF,0xFF} ;
char send_stop_palse_status[19] = {0xEE,0xB1,0x10,0x00,0x01,0x00,0x04,0xD4,0xDD,0xCD,0xA3,0xBD,0xE1,0xCA,0xF8,0xFF,0xFC,0xFF,0xFF} ;
char send_stop_status[15] = {0xEE,0xB1,0x10,0x00,0x01,0x00,0x04,0xBD,0xE1,0xCA,0xF8,0xFF,0xFC,0xFF,0xFF} ;
char send_post_status[15] = {0xEE,0xB1,0x10,0x00,0x01,0x00,0x04,0xB1,0xA8,0xB9,0xA4,0xFF,0xFC,0xFF,0xFF} ;
char read_buf[10]={0} ;

static  char  rename_passwd_flag = 0 ;   //进入到修改 密码标志
static  char* saved_password ;
static  char  passwd_buf[6] = {0};
static  int   passwd_counter = 0 ;

void*  uart_thread(void* arg) ;
void*  send_post_thread(void* arg) ;  //发送报工的线程
static  char  ALARM_POST_FLAG = 0 ;
static  char  KEY_POST_FLAG = 0 ;
static  char  event_temp[5] = {0} ;

static int machine_fd = 0 ;
static void get_machineid_for_ip(void)
{
	int ret = 0 ;
	char machine_id[2] = "00" ;

	machine_fd = open("machine.txt", O_RDONLY) ;
	if(machine_fd > 0)
	{
		ret = read(machine_fd, machine_id, 2) ;
		set_machine_id(atoi(machine_id)) ;  //设置机器设备码 文件序号
		close(machine_fd) ;
		//set ipadress
		if(set_ipadress()==1)
		{
			IPSTATUS = SETOK ;
		}

	}

}
static int  uart_init()
{
	uart_fd = open("/dev/ttyO2",O_RDWR) ;
	if(uart_fd == -1){
       printf("open /dev/ttyO2 fail \n") ; 
	   return -1;
	}else{
       printf("---uart has been opend---\n") ;
	}
	tcgetattr(uart_fd,&termold);
	tcgetattr(uart_fd,&termnew);
	cfmakeraw(&termnew);
	cfsetspeed(&termnew,B115200);  //{115200,460800,921600}
    tcsetattr(uart_fd,TCSANOW,&termnew);

	pthread_t  uart_pthread ;
	pthread_t  post_pthread ;
	pthread_create(&uart_pthread,  NULL,  uart_thread,  (void*)uart_fd);
	pthread_create(&post_pthread,  NULL,  send_post_thread,  (void*)0);

	return 0 ;
}
static void getSystemTime(void)
{
	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo=localtime(&rawtime);
	//strftime(systemtime,30,"\n%Y-%m-%d %I:%M:%S\n",timeinfo);  //12小时制
	strftime(systemtime,30,"\n%Y-%m-%d %H:%M:%S\n",timeinfo);  //24小时制
	///strftime(systemtime,30,"\n%Y-%m-%d %I:%M:%S\n",timeinfo);
	//strftime(systemtime,80,"\nsystemtime :\n%Y-%m-%d  %I:%M:%S\n",timeinfo);
#ifdef   TIME_DEBUG
	printf("\n-------------------------------\n") ;
	printf("%s\n", systemtime) ;
	printf("-------------------------------\n") ;
	for(int i=0;i<20;i++)
	{
		printf(" i = %d   %c \n",i,systemtime[i]);
	}
#endif
}

//
void reload_post_buf_event(void)
{
	//events_id 重新加载状态
	if(Event_ID==MV1)
	{
		post_buf[40] = 'M' ;  post_buf[41] = 'V' ;  post_buf[42] = '1' ;
	}
	else if(Event_ID==MV2)
	{
		post_buf[40] = 'M' ;  post_buf[41] = 'V' ;  post_buf[42] = '2' ;
	}
	else if(Event_ID==SPS)
	{
		post_buf[40] = 'S' ;  post_buf[41] = 'P' ;  post_buf[42] = 'S' ;
	}
	else if(Event_ID==SPE)
	{
		post_buf[40] = 'S' ;  post_buf[41] = 'P' ;  post_buf[42] = 'E' ;
	}
	else 
	{
		post_buf[40] = 'M' ;  post_buf[41] = 'V' ;  post_buf[42] = '3' ;
	}

}
//copy right leatest dates
void   lateast_date_copyright()
{
	char count_buf[14] = {0};

	/*************************************************
	 *  准备发送到串口屏的数据  当前时间  当前工件数量 
	 *************************************************/
	//time
	time_buf[7] = systemtime[12] ;             
	time_buf[8] = systemtime[13] ;
	time_buf[10] = systemtime[15] ;
	time_buf[11] = systemtime[16] ;
    sprintf(count_buf,"%d",POST_COUNT) ; //int to string
	memcpy(&send_count_buf[7], count_buf, sizeof(count_buf)) ;
	/*************************************************
	 * 准备发送到服务器的数据 
	 *************************************************/
    //id    把id格式为XXXXXXXXXXXX 12byte 不够0补齐
	sprintf(post_id_buf, "%012d ", POST_COUNT) ; 
    //count 把工件数量格式为XXXXXXXXXXXXXX 14byte 不够0补齐 
	memset(post_Qty,'0',14) ;
	sprintf(post_Qty, "%014d ", POST_COUNT) ;
	//now time
    memcpy(post_time_buf,&systemtime[1],19) ;       //copyright time

    //events_id
	if(Event_ID==MV1){
          post_eventcode_buf[7] = 'M' ;  post_eventcode_buf[8] = 'V' ;  post_eventcode_buf[9] = '1' ;
	}
	else if(Event_ID==MV2){
          post_eventcode_buf[7] = 'M' ;  post_eventcode_buf[8] = 'V' ;  post_eventcode_buf[9] = '2' ;
	}
	else if(Event_ID==SPS){
          post_eventcode_buf[7] = 'S' ;  post_eventcode_buf[8] = 'P' ;  post_eventcode_buf[9] = 'S' ;
	}
	else if(Event_ID==SPE){
          post_eventcode_buf[7] = 'S' ;  post_eventcode_buf[8] = 'P' ;  post_eventcode_buf[9] = 'E' ;
	}
	else {
          post_eventcode_buf[7] = 'M' ;  post_eventcode_buf[8] = 'V' ;  post_eventcode_buf[9] = '3' ;
	}
	/* post buf ready   */ 
    memset(post_buf,0,sizeof(post_buf)) ;
    memcpy(post_buf,post_id_buf,sizeof(post_id_buf)) ;
    memcpy(&post_buf[13],post_time_buf,sizeof(post_time_buf)) ;
    memcpy(&post_buf[13 + 20],post_eventcode_buf,sizeof(post_eventcode_buf)) ;
    memcpy(&post_buf[13 + 20 + 11],post_code,sizeof(post_code)) ;
    memcpy(&post_buf[13 + 20 + 11 + 31],post_Qty,sizeof(post_Qty)) ;
    memcpy(&post_buf[13 + 20 + 11 + 31 + 15],post_errorcdode_buf,sizeof(post_errorcdode_buf)) ;

    //报工完成 保存数据
	SystemPowerOff_ParemerWrite((int)Event_ID, post_Qty) ;
}

static  void  post_entry(enum POST_EVENT post_bit, char post_flag)
{
	int ret = 0  ;

	getSystemTime() ;
	//save event_id
    Event_ID = post_bit ;

	//latest time  
    lateast_date_copyright() ;      //all now  dates 

    //send to uart断网重新发送，不发送到串口屏
	//if((re_login==0) || (post_bit==MV1))  
	if((re_login==0) || post_flag==2) //按键按下 或者正常报工都会显示 
	{
		ret = write(uart_fd,time_buf,sizeof(time_buf)) ;
		ret = write(uart_fd,send_count_buf,sizeof(send_count_buf)) ;
	}
    //
	if(post_flag ==1)
	{
		ALARM_POST_FLAG = 1 ;
	}
	else
	{
		if(event_temp[0]==0)//如果发送的数据空，已经被清零 ，表示没有数据在队列里面
		{
			event_temp[0] = Event_ID ;
		}
		else
		{	
			if(event_temp[1]==0) //工作队列有一个已经缓存，但是小于两个
			{
				event_temp[1] = Event_ID ;
			}
			else  //已经缓存了两个
			{
				if(event_temp[2]==0) //但是 > 两个
				{
				 event_temp[2] = Event_ID ;
				}
				else
				{
				 event_temp[3] = Event_ID ;
				}
			}
		}
		KEY_POST_FLAG = 1 ;
	}

}
//evens from uart
static void  hmi_cmd_date_entry(char* pbuf)
{
	int ret = 0 ;
	int i = 0 ;

	if(eth0_connect_status == eth0_down)
		return ;
    //手动在休息 直接退出
	if(sleep_after_stop==1)
		return;

	delay_for_palse_flag = 1 ;  /////////临时加的  之前SPS  MV3没有 按键按下标志位

	if(pbuf[0]==0xbc) //开始/清零
	{     
		if(Event_ID == MV3)  //只有结束后 才可以清零
		{
			alarm(200) ; //10-180  2020-06-09
			ret = write(uart_fd,send_start_status,sizeof(send_start_status)) ;
			POST_COUNT = 0 ;
			target_flag = 0 ;
			sleep_after_stop = 1;
			//防止被信号打断
			for(i=0;i<180;i++)
			{
				sleep(1);
			}
			alarm(10) ; //   10-180-10  
			sleep_after_stop = 0 ;
			//ioctl(driver_misc_fd,STATUS_MV1,0) ;  //1,0
			post_entry(MV1, 2) ;
		}
	}
	else if(pbuf[0]==0xbd)
	{ 
		//status is palse
		if(Event_ID!=MV3 && Event_ID!=SPS)
		{
			alarm(alarm_time) ; //重新设置时间 避免此时正在报工
			ret = write(uart_fd,send_palse_status,sizeof(send_palse_status)) ;
			//ioctl(driver_misc_fd,STATUS_SPS,0) ;  //1,0
			post_entry(SPS, 2) ;
		
		}
	}
	else if(pbuf[0]==0xcd)
	{ 
		//暂停结束
		if(Event_ID==SPS)
		{
			alarm(10) ;
			ret = write(uart_fd,send_stop_palse_status,sizeof(send_palse_status)) ;
			//ioctl(driver_misc_fd,STATUS_SPE,0) ;  //1,0
			post_entry(SPE, 2) ;
		
		}
	}
	else if(pbuf[0]==0xbe) //结束 
	{   
		if((Event_ID == MV3)||(Event_ID == SPS))//已经暂停，必须先暂停结束 才可以结束
			return ;
	
		alarm(alarm_time) ;//重新设置时间 避免此时正在报工
		if(POST_COUNT>=target) //已经达到目标
		{
			ret = write(uart_fd,send_stop_status,sizeof(send_stop_status)) ;
			//ioctl(driver_misc_fd,STATUS_MV3,0) ;  //1,0
			post_entry(MV3, 2) ;
		}
		else  
		{
			char finish_menu[9] = {0xEE,0xB1,0x00,0x00,0x06,0xFF,0xFC,0xFF,0xFF} ;
			ret = write(uart_fd, finish_menu, sizeof(finish_menu)) ;  //弹窗提示
		}
	}
	else if(pbuf[0]==0xb5) //没有到达目标确认结束 
	{
		if((Event_ID == MV3)||(Event_ID == SPS))//已经暂停，必须先暂停结束 才可以结束
			return ;
		alarm(alarm_time) ;//重新设置时间 避免此时正在报工
		ret = write(uart_fd,enter_buf,sizeof(enter_buf)) ; //返回主页面
		ret = write(uart_fd,send_stop_status,sizeof(send_stop_status)) ; //show end
		//ioctl(driver_misc_fd,STATUS_MV3,0) ;  //1,0
		//2020-0816自动发送 间隔
		//sleep_after_stop = 1 ;
		//sleep(60) ; //自动结束后 1分钟再发  2020-06-16  按下结束后 停止一分钟
		//alarm(alarm_time) ;//重新设置时间 避免此时正在报工
		//sleep(120) ;  //2020-06-16
		//sleep_after_stop = 0 ;
		post_entry(MV3, 2) ;
	
	}
	else
	{
		//当在主界面接收不到控制指令，可能接收错误或者屏幕被异常拔出的
		//如果是收到非控制指令，直接切换界面  这里很重要  加在这里效率比较高
		char  not_cmd_key_buf[16] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x2d,0xae,0xbb,0xc3,0xc4,0xc2} ;  //all key 0~9 ...
		char i = 0 ;
		for(i=0;i<16;i++)
		{
			if(read_buf[0] == not_cmd_key_buf[i])
			{
				ret = write(uart_fd,enter_buf,sizeof(enter_buf)) ; //enter MainWindows
				UART_WINDOWS = MAIN_WINDOW ;
				passwd_counter = 0 ;
				send_passwd_buf[4] = 0x00 ; 
				send_clear_txt_buf[4] = 0x00 ;  
			}
		}
	}
}
//get lenght of string
static int get_length_of_buf (const char *string)
{
	const char *cptr = string;

	while ( *cptr )
		++cptr;
	return cptr - string; //cptr表示指向字符串的\0字符的位置，string表示指向字符串的第一个字符的位置，所以两者相减就是字符串的长度
}
//signal init
static int signal_init(void)  
{
	//open input misc   
	driver_misc_fd = open("/dev/post_misc", O_RDWR);
	if(driver_misc_fd < 0 ){
		perror("Failed to open '/dev/post_misc,can not found'");
		return -1;
	}
	printf("Open /dev/post_misc success.\n");

	//fasync
	signal(SIGIO,input_DI_handler);
	fcntl(driver_misc_fd, F_SETOWN, getpid());
	fcntl(driver_misc_fd, F_SETFL,fcntl(driver_misc_fd, F_GETFL) | FASYNC);
	//alarm	
	signal(SIGALRM, alarm_post) ;
	//alarm(alarm_time) ;
	//CTRL + C
	signal(SIGINT,stop_signal) ;
	//pipe  broken
	signal(SIGPIPE, SIG_IGN);//忽略管道破裂信号

    return 0 ;
}
void beep_ctl(void)
{
	int ret = 0 ;
	int i = 0 ,t = 0;
	char beep_buf[7] = {0xEE, 0x61, 0x62, 0xFF,0xFC, 0xFF, 0xFF} ;  

	ret = write(uart_fd, beep_buf, sizeof(beep_buf)) ; 
}
void auto_finish(void)
{
	int ret = 0 ;
	char cmd_buf[10]={0} ;

	alarm(alarm_time) ;  //重新设置时间
	cmd_buf[0] = 0xb5 ; //stop
	hmi_cmd_date_entry(cmd_buf) ;
	sleep(2) ;
	alarm(alarm_time) ;  //重新设置时间
	
	cmd_buf[0] = 0xbc ; //start
	hmi_cmd_date_entry(cmd_buf) ;	
}
int main(int argc, char** argv)  
{  
    int ret  = 0;  
	struct sockaddr_in     servaddr;  
    unsigned int  socket_stop = 1 ;	
	int skfd = 0;
	char eth0_status[2] = {0} ;  int eth0_ret = 0 ;
    char eth0_error = 0  ;

	pid_t  pid = 0 ;
	//创建子进程 用于调整时间
	pid = fork() ;
	if(pid == 0)
	{
		char time_check_flag = 0 ;
		while(1)
		{
			getSystemTime() ;
			if(systemtime[3]=='0') //2019
			{
				system("ntpdate  192.168.2.100") ;
				time_check_flag = 1 ;
				sleep(2) ;
			}
			else
			{
				if(time_check_flag==0)
				{
					system("ntpdate  192.168.2.100") ;
					sleep(2) ;
				}
				printf("\n---time has check finish---\n") ;
				exit(0) ;
				
			}
		}
	}
    //读取掉电前参数
	read_powerdown_paramer() ;

	passwd_open() ;
	saved_password = passwd_read() ;
	printf("password: %s\n",saved_password) ;
    //如果还没有获取到 则需要马上开启串口获取  
    get_machineid_for_ip() ;  //get ip
	if(machine_fd < 0)
	{
		uart_init() ;
		ret = write(uart_fd,machine_id_buf,sizeof(machine_id_buf)) ; //去掉密码 直接进入id界面
		//ret = write(uart_fd,start_UI_buf,sizeof(start_UI_buf)) ;             //reset start MainWindows
		//ret = write(uart_fd,send_clear_txt_buf,sizeof(send_clear_txt_buf)) ; //clear txt
	}
	//open input misc
	if(signal_init()<0){
		return -1 ;
	}
	//启动 直接关闭报警设备
	ioctl(driver_misc_fd, DISCONNECT, 0) ; //off 
    ioctl(driver_misc_fd, BEEP_ON, 0) ;  
	ioctl(driver_misc_fd, STATUS_MV1,0) ;  //关闭报警
    /*
	 *  先获取到IP地址
	 */
	while(IPSTATUS ==  NOSET){
		sleep(1) ;
	}
	sleep(15) ;  //10

	alarm(10) ;  ///////////////////
	system("sync") ;

	skfd = open("/sys/class/net/eth0/carrier", O_RDONLY) ;
	if(skfd < 0)
		printf("cat eth0 error!\n") ;
    else  
		printf("cat eth0 success!\n") ;

	do
	{
		//ftp_sockfd = login() ;
		if(!ftp_sockfd)
			ftp_sockfd = mount_login() ;
		else
			ftp_sockfd = remount_login() ;

		printf("==ready to login %d==\n", ftp_sockfd) ;

		if(!ftp_sockfd)
		{
			printf("\n===login finish %d===\n") ;
		}
		else
		{
			lseek(skfd, 0, SEEK_SET) ;
			memset(eth0_status, '0' , 2) ;
			eth0_ret = read(skfd, eth0_status, 2) ;
			if(eth0_status[0] == '0') //如果是网线断了导致登录失败 则报警  否则继续登录 
			{
				for(char i=0;i<6;i++)
				{
					ioctl(driver_misc_fd, ON_CONNECT, 0) ; beep_ctl() ; 
					sleep(1) ;
					ioctl(driver_misc_fd, DISCONNECT, 0) ; beep_ctl() ; 
					sleep(1) ;
					printf("---eth0 has down please check right now---\n") ;
				}

			}

		}
		sleep(1) ;
	}while(ftp_sockfd>0); //直到挂载成功

	IPSTATUS = SETFINISH ;

	if(machine_fd > 0 )
	{
		uart_init() ;
		ret = write(uart_fd,enter_buf,sizeof(enter_buf)) ; //去掉密码了 直接进入主界面
		//ret = write(uart_fd,start_UI_buf,sizeof(start_UI_buf)) ;             //reset start MainWindows
		//ret = write(uart_fd,send_clear_txt_buf,sizeof(send_clear_txt_buf)) ; //clear txt
	}
	while(1){
		if(target_flag == 0)
		{
			if(POST_COUNT>=target)
			{
				for(char i=0;i<6;i++)
				{
					ioctl(driver_misc_fd,STATUS_SPS,0) ;  //1,0
					//	ioctl(driver_misc_fd, BEEP_OFF, 0) ;  
					sleep(5) ;
					//	ioctl(driver_misc_fd, BEEP_ON, 0) ;  
					ioctl(driver_misc_fd,STATUS_MV1,0) ;  //1,0
					sleep(1) ;
				}
				target_flag = 1 ;
			}
		}
		else if(target_flag == 1)
		{
			if(POST_COUNT>=(target + target/100))
			{
				auto_finish() ;
			}
			
		}
		
		usleep(100000) ;
		//检查网线是否拔出
		lseek(skfd, 0, SEEK_SET) ;
		memset(eth0_status, '0' , 2) ;
        eth0_ret = read(skfd, eth0_status, 2) ;
		//printf("---status eth0 %c---\n", eth0_status[0]) ;
		if(eth0_status[0] == '0')  
		{
			eth0_connect_status = eth0_down ;
			eth0_error = 1 ;
				for(char i=0;i<6;i++)
				{
					ioctl(driver_misc_fd, ON_CONNECT, 0) ; beep_ctl() ; 
					sleep(1) ;
					ioctl(driver_misc_fd, DISCONNECT, 0) ; beep_ctl() ; 
					sleep(1) ;
					printf("---eth0 has down please check right now---\n") ;
				}
		
		}
		else
		{
			if(eth0_error == 1)
			{
				for(char t=0;t<10;t++)  //2.5min
				{
					sleep(5) ;
					printf("---wait  30s for socket---\n") ;
				}
				eth0_error = 0 ;

				event_temp[0] = 0 ; event_temp[1] = 0 ; KEY_POST_FLAG = 0 ; ALARM_POST_FLAG = 0 ; 
				//
				//system("reboot") ;

			}
			if(eth0_connect_status == eth0_down)
				eth0_connect_status = eth0_up ; 
		}

	}
	close(connect_fd);
	close(socket_fd); 
	close(driver_misc_fd) ;
	close(uart_fd) ;
	return 0 ;
}  
//
void*  send_post_thread(void* arg) 
{
	int ret = 0 ;
	int temp_v = 0 ;

	while(IPSTATUS != SETFINISH )
	{
		printf("---正在等待获取ip地址---\n") ;
		sleep(2) ;
	}
    //等待时间校准	
	getSystemTime() ;
	while(systemtime[3]=='0') //2019
	{
		getSystemTime() ;
		printf("--waiting for checking time--\n") ;
		sleep(1) ;
	}
    //需要重新加载时间  因为之前已经包含不准确的时间
    lateast_date_copyright() ; 

	while(1)
	{	
		if(KEY_POST_FLAG == 1)
		{
			Event_ID = event_temp[0] ; 
			reload_post_buf_event() ;
			ret = postDatesToService(ftp_sockfd, "post_ftp", post_buf, re_login, KEY_POST_FLAG) ;  //
			re_login = 0 ;
			if(ret == -3)
			{
				printf("key send error\n") ;
				//ftp_sockfd = login() ;
				ftp_sockfd = remount_login() ;
				re_login = 1 ;

			}
			else
			{
				if(event_temp[1]!=0)
				{
					if(event_temp[2]!=0)
					{
						if(event_temp[3]!=0)
						{
							event_temp[0] = event_temp[1] ; //准备发送1
							event_temp[1] = event_temp[2] ; //event_temp[2]-->event_temp[1]
							event_temp[2] = event_temp[3] ; 
							event_temp[3] = 0 ;  //清零event_temp[3]

						}
						else
						{
							event_temp[0] = event_temp[1] ; //准备发送1
							event_temp[1] = event_temp[2] ; //event_temp[2]-->event_temp[1]
							event_temp[2] = 0 ;  //清零event_temp[2]
						}
					}
					else
					{
						event_temp[0] = event_temp[1] ; //
						event_temp[1] = 0 ; //准备发送缓存的数据
					}
				}
				else
				{
					KEY_POST_FLAG = 0 ; //按键标志也要清零
					event_temp[0] = 0 ; //clear 发送完成所有的 清零
				}
			}

		}
		else if(ALARM_POST_FLAG == 1)
		{
			ALARM_POST_FLAG = 0 ;
			ret = postDatesToService(ftp_sockfd, "post_ftp", post_buf, re_login, KEY_POST_FLAG) ;  //
			re_login = 0 ;
			if(ret == -3)
			{
				printf("alarm send error\n") ;
				//ftp_sockfd = login() ;
				ftp_sockfd = remount_login() ;
				re_login = 1 ;
				alarm(3) ;  //5
				if(delay_for_palse_flag==1)
				{
					delay_for_palse_flag = 2 ;
					Temp_Event_ID = Event_ID ;
				}
			}

		}
		usleep(10000) ;

	}
}

//get inpput
void input_DI_handler(int sig) 
{
	if(Event_ID == MV3 || Event_ID == SPS)	
	{

	}
	else
	{
		POST_COUNT++;
		printf("--  %d --\n",POST_COUNT) ;
		//show_log();
	}

}  

//alarm
void  alarm_post()
{
	int ret = 0 ;

	alarm(alarm_time) ;
	if(delay_for_palse_flag ==1){ //如果是状态改变而已  不报工  只改变状态
		delay_for_palse_flag = 0 ;
		if(Event_ID == SPS || Event_ID ==MV3){
             //因为增加了按键按下标志 所以需要这样处理
		}
		else
		{
			ret = write(uart_fd,send_post_status,sizeof(send_post_status)) ;  //show start  ,palse has finished
		}
	}
	else if(delay_for_palse_flag==2)//如果点击开始，因为filezilla休眠上报失败，则继续上报MV1
	{
		delay_for_palse_flag = 0 ;
		if(Temp_Event_ID ==  SPE) //如果暂停很久 导致重新发送 则显示报工  临时添加
		{
			ret = write(uart_fd,send_post_status,sizeof(send_post_status)) ;  //show start  ,palse has finished
		}
		post_entry(Temp_Event_ID, 2) ;  //发送失败的状态 继续发送
	}
	else
	{
		if(Event_ID == SPS || Event_ID ==MV3){

		}
		else
		{
			if(KEY_POST_FLAG == 0)
			{
				post_entry(MV2, 1) ;
				ret = write(uart_fd,send_post_status,sizeof(send_post_status)) ;  //show start  ,palse has finished
			}
		}
	}

}
/*
 * stop signal
 * CTRL + C
 */
void  stop_signal()
{
	printf("---The application will stop  all resouces will free---\n") ; 
	if(connected_flag){
	  close(connect_fd);
	}
	close(socket_fd); 
	close(driver_misc_fd) ;
	close(uart_fd) ;
	close(log_fd) ;
   
	exit(0) ;
}
//
void  passwd_check(char* pdate) ;
void  get_machine_id(char *p_idbuf) ;
void SystemRestarRecovery(void) ;
//uart  thread
void*  uart_thread(void* arg) 
{
	int ret = 0 ;
    
	while(1)
	{
		ret = read(uart_fd,read_buf,sizeof(read_buf)) ;
#ifdef   LCD_UART_DEBUG
		printf("\n value  0x%x  \n",read_buf[0]) ;
		printf("主页面是： %d\n",UART_WINDOWS) ;
#endif
		if(UART_WINDOWS == PASSWD_WINDOW)
		{
			if(machine_fd < 0)
			{
				ret = write(uart_fd,machine_id_buf,sizeof(machine_id_buf)) ; 
				UART_WINDOWS = GET_ID_WINDOW ;
			}
			else      //已经设置过 直接恢复
			{
				ret = write(uart_fd,enter_buf,sizeof(enter_buf)) ; 
				UART_WINDOWS = MAIN_WINDOW ;
			}
			SystemRestarRecovery() ;

		}
#if  0
		if(UART_WINDOWS == PASSWD_WINDOW)
		{
	
			if(read_buf[0]==0xbb)
			{
				if(strcmp(saved_password,passwd_buf)==0){
					if(machine_fd < 0)
					{
						ret = write(uart_fd,machine_id_buf,sizeof(machine_id_buf)) ; 
						UART_WINDOWS = GET_ID_WINDOW ;
					}
					else      //已经设置过 直接恢复
					{
						ret = write(uart_fd,enter_buf,sizeof(enter_buf)) ; 
						UART_WINDOWS = MAIN_WINDOW ;
					}
					SystemRestarRecovery() ;

				}else{
					ret = write(uart_fd,error_buf,sizeof(error_buf)) ; //
					ret = write(uart_fd,send_clear_txt_buf,sizeof(send_clear_txt_buf)) ; 
					passwd_counter = 0 ;
				}
			}
			else if(read_buf[0]==0xc1)  //new passwd save
			{
				passwd_write(passwd_buf) ;  //save passwd right now
				ret = write(uart_fd,enter_buf,sizeof(enter_buf)) ; //enter MainWindows
				UART_WINDOWS = MAIN_WINDOW ;
				passwd_counter = 0 ;
				send_passwd_buf[4] = 0x00 ; //跳转到控件ID为新密码的文本控件
				send_clear_txt_buf[4] = 0x00 ;
			}
			else if(read_buf[0]==0xc2)  //校验密码确认
			{
				if(strcmp(saved_password,passwd_buf)==0){ //验证密码正确后，跳转到新密码界面
					char new_passwd_buf[9] = {0xEE,0xB1,0x00,0x00,0x04,0xFF,0xFC,0xFF,0xFF} ;
					ret = write(uart_fd,new_passwd_buf,sizeof(new_passwd_buf)) ; //new  pasword  windows

					passwd_counter = 0 ;
					send_passwd_buf[4] = 0x04 ; //跳转到控件ID为新密码的文本控件
					send_clear_txt_buf[4] = 0x04 ;
                    memset(passwd_buf,0x00,sizeof(passwd_buf)) ;//////////////////////////////////////

				}else{
					//////////////////////////////////////////ret = write(uart_fd,error_buf,sizeof(error_buf)) ; //
					ret = write(uart_fd,send_clear_txt_buf,sizeof(send_clear_txt_buf)) ; 
					passwd_counter = 0 ;
				}
			}
			else if(read_buf[0]==0xc3)
			{
				send_passwd_buf[4] = 0x03 ;//改变空控件ID为密码检验文本控件
				send_clear_txt_buf[4] = 0x03 ;
				rename_passwd_flag = 1 ;   //进入到修改 密码标志  如果没有这个标志处理 来回界面切换就会异常
				ret = write(uart_fd,send_clear_txt_buf,sizeof(send_clear_txt_buf)) ; //clear txt
				passwd_counter = 0 ;
				printf("prom page 1 turn to page 2\n") ;
			}
			else if(read_buf[0]==0xc4)
			{
				send_passwd_buf[4] = 0x00 ;
				send_clear_txt_buf[4] = 0x00 ;
				rename_passwd_flag = 0 ;
				ret = write(uart_fd,send_clear_txt_buf,sizeof(send_clear_txt_buf)) ; //clear txt
				passwd_counter = 0 ;
				printf("from page 2  turn to page 1\n") ;
			}
			else
			{   //on the mainwndows aleady
				if(read_buf[0]==0xbc || read_buf[0]==0xbd || read_buf[0] == 0xcd || read_buf[0] == 0xbe){
					UART_WINDOWS = MAIN_WINDOW ;
					hmi_cmd_date_entry(read_buf) ;
				}else{
					passwd_check(read_buf) ;
				}
			}
		}
#endif
		else if(UART_WINDOWS == GET_ID_WINDOW)
		{
			get_machine_id(read_buf) ;
		}
		else
		{
			hmi_cmd_date_entry(read_buf) ;
		}
	}
}
void  get_machine_id(char *p_idbuf)
{
	static char id_flag = 0 ;
	static char last_num = 0x30 ;
	static int final_id = 0 ;
	int ret = 0 , i = 0;
	char  number_char_buf[12] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xae,0xbb} ;
	char id_buf[13]={0xEE,0xB1,0x10,0x00,0x05,0x00,0x01,0x30,0x31,0xFF,0xFC,0xFF,0xFF} ; 

	if(p_idbuf[0]!=0xd2)
	{
		if(p_idbuf[0] == 0xae)
		{
			id_buf[7] = 0x30 ;
			id_buf[8] = 0xFF ;
			id_buf[9] = 0xFC ;
			id_buf[10] = 0xFF ;
			id_buf[11] = 0xFF ;
			
			last_num = 0x30 ;
			ret = write(uart_fd, id_buf, 12) ;
			id_flag = 0 ;

		}
		else
		{
			if(id_flag==0)
			{
				id_buf[7] = p_idbuf[0] ;
				id_buf[8] = 0xFF ;
				id_buf[9] = 0xFC ;
				id_buf[10] = 0xFF ;
				id_buf[11] = 0xFF ;
				//save 保存第一次的输入
				last_num = p_idbuf[0] ;
				ret = write(uart_fd, id_buf, 12) ;
				id_flag++;
				//把数字的1ASCLL的16进制 重新转化为数字
                for(i=0;i<=9;i++)
				{
					if(p_idbuf[0] == number_char_buf[i])
					{
						final_id = i ;
					    break;
					}
				}
			}
			else
			{
				id_buf[7] = last_num ;
				id_buf[8] = p_idbuf[0] ;
				id_buf[9] = 0xFF ;
				id_buf[10] = 0xFC ;
				id_buf[11] = 0xFF ;
				id_buf[12] = 0xFF ;
				ret = write(uart_fd, id_buf, 13) ; 
				id_flag = 0 ;
				//把数字的1ASCLL的16进制 重新转化为数字
                for(i=0;i<=9;i++)
				{
					if(p_idbuf[0] == number_char_buf[i])
					{
						final_id = final_id * 10 + i ;
					    break;
					}
				}
			}
		}
	}
	else
	{
		int machinefd = 0 ;
		char machine_buf[2] = "00" ;
		ret = write(uart_fd,enter_buf,sizeof(enter_buf)) ; //enter MainWindows
		UART_WINDOWS = MAIN_WINDOW ;
		printf("序号是:%d\n", final_id) ;
		set_machine_id(final_id) ;
		
		machinefd = open("machine.txt", O_WRONLY | O_CREAT, 666) ;
		sprintf(machine_buf, "%02d", final_id) ; 
        //write machine id to files
		ret = write(machinefd, machine_buf, 2) ;
		close(machinefd) ;
		//set ipadress
		if(set_ipadress()==1)
		{
			IPSTATUS = SETOK ;
		}
		//fflush(stdout) ;
	}

}
void  passwd_check(char*  pdate)
{
	int ret = 0 ;
	char i = 0 ;
	char  passwd_number_char_buf[39] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39 \
		                               ,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a \
									   ,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54 \
									   ,0x55,0x56,0x57,0x58,0x59,0x5a,0x20,0x2d} ;
	
	if(pdate[0]==0xae){
		if(passwd_counter>0){
			passwd_counter-- ;
			if(passwd_counter==0) 
			{
				ret = write(uart_fd,send_clear_txt_buf,sizeof(send_clear_txt_buf)) ; //clear txt
				return ;
			}
			send_passwd_buf[7+passwd_counter] = 0x2A ; //clear bit  0x2A  or anny others caracter
			memcpy(send_temp,send_passwd_buf,7) ;  //0-6   head
			send_temp[6 + passwd_counter] = 0x2A ;  // *
			memcpy(&send_temp[6 + passwd_counter + 1],&send_passwd_buf[13],4) ;  //x-17   end
			ret = write(uart_fd,send_temp,12 + passwd_counter) ; //show passwd
		}else{
			ret = write(uart_fd,send_clear_txt_buf,sizeof(send_clear_txt_buf)) ; //clear txt
			passwd_counter = 0 ;
		}
	}else{
		if(passwd_counter <= 5){
			//check input     
			for(i=0;i<39;i++){
				if(pdate[0] == passwd_number_char_buf[i]){
					send_passwd_buf[7+passwd_counter++] = passwd_number_char_buf[i] ;
					break;
				}
			}

			memcpy(passwd_buf,&send_passwd_buf[7],6) ;
#ifdef   LCD_UART_DEBUG
			printf("page:%d",rename_passwd_flag) ;
			printf("input password:\n") ;
			for(int i=0;i<6;i++){
				printf("%x ",passwd_buf[i]) ;	
			}
#endif
			memcpy(send_temp,send_passwd_buf,7) ;  //0-6  head
			send_temp[6 + passwd_counter] = 0x2A ;  // *
			memcpy(&send_temp[6 + passwd_counter + 1],&send_passwd_buf[13],4) ;  //x-17
			ret = write(uart_fd,send_temp,12 + passwd_counter) ; 
		}else{ 
			ret = write(uart_fd,error_buf,sizeof(error_buf)) ; //passwd error	
			ret = write(uart_fd,send_clear_txt_buf,sizeof(send_clear_txt_buf)) ; //clear txt
			rename_passwd_flag = 0 ;            //recovery to mainwindows
			send_passwd_buf[4] = 0x00 ;         //text id:  palease inter...
			send_clear_txt_buf[4] = 0x00 ;
			passwd_counter = 0 ;
		}
	}
}
//显示工单号
void show_job_number(char* job_number)
{
	char  number[36] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" ;
	char  number_char_buf[38] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39, \
								 0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A, \
								 0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,0x54, \
								 0x55,0x56,0x57,0x58,0x59,0x5A,0xae,0xbb} ;
	char id_buf[23]={0xEE,0xB1,0x10,0x00,0x01,0x00,0x08, \
		             0x35,0x32,0x31,0x30,0x30,0x30,0x30, \
					 0x30,0x30,0x31,0x30,0x30,0xFF,0xFC,0xFF,0xFF} ; 
	char i = 0 , j = 0, cnt = 0;
    int  ret = 0 ;
#if 0
	printf("----------------\n") ;
	for(i=0;i<10;i++)
	{
		printf("%c\n", job_number[i]) ;
	}
	printf("----------------\n") ;
	ret = write(uart_fd, id_buf, 17) ;      
#endif
	for(i=0;i<12;i++)
	{
		for(j=0;j<36;j++)
		{
			if(job_number[i] == number[j])
			{
				id_buf[7 + cnt] = number_char_buf[j] ;
				cnt++ ;
				break;
			}
		}
	}
	ret = write(uart_fd, id_buf, 23) ;      

}
//无任务卡
void NoTask(void)
{
	int ret = 0 ;
	char id_buf[23]={0xEE,0xB1,0x10,0x00,0x01,0x00,0x08, \
		             0x35,0x32,0x31,0x30,0x30,0x30,0x30, \
					 0x30,0x30,0x31,0x30,0x30,0xFF,0xFC,0xFF,0xFF} ; 

	memcpy(&id_buf[7], "   NO TASK  ", 12) ;
	ret = write(uart_fd, id_buf, 23) ;      
}
void SystemRestarRecovery(void)
{
	int ret = 0 ;
	if(Event_ID == MV3)
		ret = write(uart_fd,send_stop_status,sizeof(send_stop_status)) ;   //结束
	else if(Event_ID == SPS)
		ret = write(uart_fd,send_palse_status,sizeof(send_palse_status)) ; //暂停
	else
		ret = write(uart_fd,send_post_status,sizeof(send_post_status)) ;  //报工
}
