#include "cmi_at155_application.h"
#include "client_ftp.h"

//#define  DEBUG_PRINT
#define    PARAMER_SAVE_MODE 
static int fd = 0 ;
static char machine_id[2] = "00" ; //机器设备码 1-20
static unsigned int  send_id = 0 ; //发送序号

char p_buf[100] = "" ;  //存放配置文件数据
static char SYSTEM_START_FLAG = 0 ;  
int   GetParamerSuccessFlag = 1  ; //get success paramer default=success 

static char yesterday[9] = "20190711" ;  //日期变化后此处id

extern  unsigned long target ;
extern int  alarm_time ; //上报时间
extern enum  POST_EVENT   Event_ID  ;
extern unsigned  long  POST_COUNT ;

static char taskcar_number[30] = "00" ;
/*
 * mount ---  umount
 */
int  mount_login(void)
{
	return system("mount -t cifs //192.168.2.100/ftp_files  /mnt/ -o username=abc,password=123") ;
}
int  remount_login(void)
{
	system("umount  /mnt/") ;  
	sleep(1) ;
	system("sync") ;
	return system("mount -t cifs //192.168.2.100/ftp_files  /mnt/ -o username=abc,password=123") ;

}

int ftpget_mes_dll(void) //success return  1
{
	int ret = 0 ;
	//char cmd[56] = "ftpget -u windows-ftp -p 111111 192.168.2.100 MESXX.dll" ;
	char cmd[26] = "cp /mnt/config/*XX_*   ./" ;

	cmd[16] = machine_id[0] ; //
	cmd[17] = machine_id[1] ;

	ret = system(cmd) ;  

	if(ret !=0)
	{
		remount_login();
		sleep(2) ;
		ret = system(cmd) ;  //重新复制
		if(ret !=0)
			return  0 ;  //fail
	}
	
	system("sync") ;

	return 1 ;   //success 

}

int delet_task_car(void)
{
	int ret = 0 ;
	char rmparamer[40] = "mv  /mnt/config/*XX_*   /mnt/config/old" ;
    //delet 
	rmparamer[17] = machine_id[0] ; //
	rmparamer[18] = machine_id[1] ;

	ret = system(rmparamer) ;

	system("sync") ;

	return 0;
}
/*
 *retuen -1 ,  do reset time
 *return 0  ,  do nothing
 */
int get_paramer_dll()
{
	int ret = 0 ;
	int paramer_fd = 0 ;
	int new_fd = 0 ;
	char new_paramer_buf[100] = "" ;

	system("cp  date-stb-old.dll  history.dll") ; //防止不存在MES*的时候 有文件可以读取
	//system("mv  MES*  history.dll") ; //上次的备份 防止出错的时候直接使用  同时清除上一个
	system("rm  MES* && sync") ;

	if(ftpget_mes_dll())
	{
		system("mv  MES*  date-stb.dll") ;
		GetParamerSuccessFlag = 1 ;
		system("touch  success.txt") ;
	}
	else //fail  read history
	{
		system("mv  history.dll  date-stb.dll") ;
		GetParamerSuccessFlag = 0 ;
		system("rm  success.txt") ;
		target = 60000 ; //获取不到的时候 防止以为结束  默认六万
	}

	system("sync") ;

	paramer_fd = open("date-stb-old.dll", O_RDONLY) ;

	if(paramer_fd < 0)   //不存在，开始获取
	{
		printf("\n==打开配置文件失败  准备重新获取配置文件==\n") ;
		//system("ftpget -u windows-ftp -p 111111 192.168.2.1 date-stb.dll") ;
		system("cp date-stb.dll  date-stb-old.dll") ;   //备份一份
		system("sync") ;
		paramer_fd = open("date-stb.dll", O_RDONLY) ;
		if(paramer_fd < 0)
		{
			return 0;
		}
		printf("\n读取配置文件>>>>>>>>>>>\n") ;
		ret  = read(paramer_fd, p_buf, 98) ;
		if(ret < 0)
		{
			printf("\n配置文件为空，使用默认参数\n") ;
			close(paramer_fd) ;
			return 0 ;
		}

		system("cp date-stb.dll  date-stb-old.dll") ;   //备份一份
		system("sync") ;

		close(paramer_fd) ;
		return -1 ;  //reset......
	}
	//printf("\n读取配置文件>>>>>>>>>>>\n") ;
	//已经存在查看是否更新
	ret  = read(paramer_fd, p_buf, 98) ;
	if(ret < 0)
	{
		printf("\n配置文件为空，使用默认参数\n") ;
		return 0 ;
	}
	else
	{
		new_fd = open("date-stb.dll", O_RDONLY) ;
		if(new_fd < 0)
		{
			close(paramer_fd) ; 
			return 0 ;         //文件错误，重新设置
		}
		ret = read(new_fd, new_paramer_buf, 98) ;
		if(strcmp(p_buf, new_paramer_buf)==0) //没有更新，则不重新设置
		{

			close(paramer_fd) ;
			close(new_fd) ;
			//printf("\n没有更新，不需要重新设置\n") ;
			return 0 ;
		}
		else
		{
#ifdef   DEBUG_PRINT
			printf("\n old: %s   len: %d\n", p_buf, ret) ;
			printf("\n new: %s   len: %d\n", new_paramer_buf, ret) ;
#endif
		   	close(paramer_fd) ;
			close(new_fd) ;
			if(ret == 98)
			{
				//拷贝新数据，并备份
				memcpy(p_buf, new_paramer_buf, 98) ;
				system("cp date-stb.dll  date-stb-old.dll") ;   //备份一份
				system("sync") ;

				printf("\n有更新，重新设置\n") ;
			}
			return -1 ;  //reset ......

		}
	}

	////printf("\nparamer: %s   len: %d\n", p_buf, ret) ;
	close(paramer_fd) ;

	return 0 ;
}
void fresh_paramer(void)
{
    char target_number[10] = "00000"  ;
		char n = 0 ;
		char taskcar_temp[30] = "00" ;

	if(SYSTEM_START_FLAG == 0)
	{
		int  finish_read_fd = 0 ;
		int  last_fd = 0 ;
		int  ret = 0 ;

		finish_read_fd = open("success.txt", O_RDONLY) ;
		if(finish_read_fd < 0) //掉电前读取失败  需要继续
		{
			get_paramer_dll() ;
		}
		else   //已经得到 直接获取
		{
			close(finish_read_fd) ;
			last_fd = open("date-stb-old.dll", O_RDONLY) ;
			ret  = read(last_fd, p_buf, 98) ;
			close(last_fd) ;
		}

	}
	else
	{
		get_paramer_dll() ; //get paramer right now
	}
	//更新上报时间
	if(atoi(&p_buf[89])>0)
		alarm_time = atoi(&p_buf[89]) ;
	//target
	memcpy(target_number, &p_buf[7], 5) ; ////////xxxxxxxxxxx
	if(atoi(target_number)>0)
	{
		if(GetParamerSuccessFlag)
			target = atoi(target_number) ;
		else
			target = 60000 ; //获取不到的时候 防止以为结束  默认六万
	}
	else
		target = 60000;

	if(GetParamerSuccessFlag)
	{
		show_job_number(&p_buf[44]) ;

		memset(taskcar_number, 0, 22) ;
		memset(taskcar_temp, 0, 22) ;
		memcpy(taskcar_temp,&p_buf[44], 22) ; //工单号

        //最后是01
		if(taskcar_temp[20] == '0')
		{
			taskcar_temp[20] = taskcar_temp[21] ;
			taskcar_temp[21] = '\n' ;
			taskcar_temp[22] = '\n' ;
		}
        //1-12
		memcpy(taskcar_number,taskcar_temp, 12) ; 
		taskcar_number[12] = '-' ;
		if(taskcar_temp[12] == '0')
		{
			taskcar_number[13] = taskcar_temp[13] ; 
			taskcar_number[14] = '-' ; 
			taskcar_number[15] = taskcar_temp[14] ; 
			taskcar_number[16] = taskcar_temp[15] ; 
			taskcar_number[17] = '-' ; 
			taskcar_number[18] = taskcar_temp[16] ; 
			taskcar_number[19] = taskcar_temp[17] ; 
			taskcar_number[20] = taskcar_temp[18] ; 
			taskcar_number[21] = taskcar_temp[19] ; 
			taskcar_number[22] = '-' ; 
			taskcar_number[23] = taskcar_temp[20] ; 
			taskcar_number[24] = taskcar_temp[21] ; 
		}
		else
		{
			taskcar_number[13] = taskcar_temp[12] ; 
			taskcar_number[14] = taskcar_temp[13] ; 
			taskcar_number[15] = '-' ; 
			taskcar_number[16] = taskcar_temp[14] ; 
			taskcar_number[17] = taskcar_temp[15] ; 
			taskcar_number[18] = '-' ; 
			taskcar_number[19] = taskcar_temp[16] ; 
			taskcar_number[20] = taskcar_temp[17] ; 
			taskcar_number[21] = taskcar_temp[18] ; 
			taskcar_number[22] = taskcar_temp[19] ; 
			taskcar_number[23] = '-' ; 
			taskcar_number[24] = taskcar_temp[20] ; 
			taskcar_number[25] = taskcar_temp[21] ; 
		}
	}
	else
	{
		memset(taskcar_temp, ' ', 26) ;
		memset(taskcar_number, ' ', 26) ;
		taskcar_number[0] = 'x' ;
		taskcar_number[1] = 'x' ;
		taskcar_number[2] = 'x' ;
		taskcar_number[3] = '\r' ;
		taskcar_number[4] = '\n' ;

		NoTask() ;
	}
		
}
int postDatesToService(int sockfd, char* pathname, char* inputbuf, int flag, char mode) 
{
	static char save_status[4] = "000" ;
	//状态参数传入的时候是指针，如果操作太快，指针内容会被修改，设置临时缓存
	char save_temp[4] = "000" ;

	int ret = 0 ;
	char remote_pathname[45] = "5120010001101201906000001_" ;
    
    char local_pathname[45] = "";
	char send_day[9] = "20190527" ;   //发送日期
	char filename_id[5] = "0001" ;     //文件序号

	char sendFilesString[134] = "  " ;

	save_temp[0] = inputbuf[40] ; 
	save_temp[1] = inputbuf[41] ; 
	save_temp[2] = inputbuf[42] ; 
#ifdef    DEBUG_PRINT
    for(ret=0;ret<100;ret++)
	printf("senddate: %c %d\n",inputbuf[ret],ret) ;
#endif
    if(SYSTEM_START_FLAG == 0)
	{   
		fresh_paramer() ;    //刷新最新数据
		//printf("时间设置为:%d \n",alarm_time) ;
		memcpy(yesterday,&inputbuf[13],4) ; //2019
		memcpy(&yesterday[4],&inputbuf[18],2) ; //05
		memcpy(&yesterday[6],&inputbuf[21],2) ; //27
		SYSTEM_START_FLAG = 1 ;
	}
	/***临时修改的id****/
	remote_pathname[23] = machine_id[0] ;
	remote_pathname[24] = machine_id[1] ;
	//
	memcpy(send_day,&inputbuf[13],4) ; //2019
	memcpy(&send_day[4],&inputbuf[18],2) ; //05
	memcpy(&send_day[6],&inputbuf[21],2) ; //27
	if(strncmp(yesterday, send_day, 8)!=0)
	{
		send_id = 0 ;
		memcpy(yesterday, send_day, 8) ;
	};
	if(flag==0) //重新登陆 不增加
		send_id++ ;
    sprintf(filename_id,"%04d",send_id) ; //整数转化为字符串
	//初始化文件名
	strcat(remote_pathname,send_day);
	strcat(remote_pathname,"_");
	strcat(remote_pathname,filename_id);
	strcat(remote_pathname,".txt");
	memcpy(local_pathname, remote_pathname, sizeof(remote_pathname)) ;
	
	fd = open(remote_pathname, O_RDWR | O_CREAT, 666) ;
    lseek(fd,0,SEEK_SET) ;

    /** 格式化数据 拷贝  把空格换位回车  **/
    memset(sendFilesString,'0',120) ;
	memcpy(sendFilesString,inputbuf,12);  //12
	sendFilesString[12] = '\r' ;
	sendFilesString[13] = '\n' ;

	memcpy(&sendFilesString[14],&inputbuf[13],19);  //19
	sendFilesString[33] = '\r' ;
	sendFilesString[34] = '\n' ;

	memcpy(&sendFilesString[35],&inputbuf[33],10);  //10
	sendFilesString[45] = '\r' ;
	sendFilesString[46] = '\n' ;
	
	memcpy(&sendFilesString[47],&inputbuf[44],30);  //30
	sendFilesString[69] = remote_pathname[22] ;  //修改内容为和文件名一致
	sendFilesString[70] = remote_pathname[23] ;
	sendFilesString[71] = remote_pathname[24] ;

	sendFilesString[77] = '\r' ;
	sendFilesString[78] = '\n' ;

	memcpy(&sendFilesString[79],&inputbuf[75],14);  //
	sendFilesString[93] = '\r' ;
	sendFilesString[94] = '\n' ;

	memcpy(&sendFilesString[95],&inputbuf[90],8);  //
	sendFilesString[103] = '\r' ;
	sendFilesString[104] = '\n' ;

    //获取任务卡以后 在发送  2020-06-02
	if((save_temp[0]=='M' && save_temp[1]=='V' && save_temp[2]=='1')||(GetParamerSuccessFlag==0))
		fresh_paramer() ;

	//发送数据
	memcpy(&sendFilesString[105],&taskcar_number[0],26);  //工单号

	ret = write(fd, sendFilesString, 132) ; //inputbuf  105
	//临时加的 判断是否出现空白文件
	if(ret < 1)
		show_log() ;
	
	system("sync") ;
	
	if(ret > 0){
		printf("最新数据写入完成\n") ;
	}
    //---
    lseek(fd,0,SEEK_SET) ;
	close(fd) ;
	
	//char copy_buf[60] = "cp 5120010001101201907000001_20190924_0101.txt    /mnt/" ;

    //如果是结束 则删除之前的任务卡
	if((save_temp[0] == 'M' &&  save_temp[1] == 'V' && save_temp[2] == '3'))	
		delet_task_car() ; //删除旧的任务卡

	//copy files to /mnt/
	system("cp  512*  /mnt/") ; 

	ret = system("ls  /mnt/  | grep  new.bat") ;
	if(ret != 0)
	{
		system("cp  512*   /media/") ;
		remount_login() ;
		system("cp  /media/*   /mnt/  -f  &&  rm /media/* -f") ;
		system("sync") ;
		close(fd) ;
		return -3 ;
	}
	else
	{
		//finish  刷新最新数据
		//if((save_status[0] == 'M' &&  save_status[1] == 'V' && save_status[2] == '1') || \
			(GetParamerSuccessFlag==0)) //结束后 或者启动掉电后需要程序获取的情况 
		///if((save_temp[0]=='M' && save_temp[1]=='V' && save_temp[2]=='1')||(GetParamerSuccessFlag==0)) 
		///{
		///	fresh_paramer() ;    
		///}
		sleep(2) ; //等待传输完成
		system("sync") ;
		system("rm 51200100011012019*") ;
		//if(GetParamerSuccessFlag == 1) //获取到后，才会停止获取，否则继续获取
		//{
			//保存状态，如果是MV1  下一次直接读取配置 
		//	save_status[0] = save_temp[0] ; 
		//	save_status[1] = save_temp[1] ; 
		//	save_status[2] = save_temp[2] ; 
		//}

	}
#if  0
	ret = ftp_up(sockfd, remote_pathname, local_pathname);
	if(ret == -2)
	{
		//发送失败 返回-2  启用重连
		ftp_quit(sockfd) ; //quit first
		close(fd) ;
		return -3 ;
	}
#endif
	printf("\n===login finish===\n") ;
	return 0 ;

}
void set_machine_id(int machineid)
{
    sprintf(machine_id, "%02d", machineid) ; //设置机器设备码 文件序号
}

int read_powerdown_paramer()
{
	char  saved_buff[25] = "0" ;
	char postcount[15] = "0";
	int  p_fd = 0 ;
	char file_id[5] = "0";
	int  ret = 0 ;

	p_fd = open("paramer.txt", O_RDONLY) ;

	if(p_fd < 0)
	{
		return -1 ;
	}

	ret = read(p_fd, saved_buff, sizeof(saved_buff)) ;
	lseek(p_fd,0,SEEK_SET) ;

	if(saved_buff[0] == '1')
	{
		Event_ID = MV1 ;
	}
	else if(saved_buff[0] == '2'){
		Event_ID = MV2 ;
	}
	else if(saved_buff[0] == '3'){
		Event_ID = SPS ;
	}
	else if(saved_buff[0] == '4'){
		Event_ID = SPE ;
	}
	else {
		Event_ID = MV3 ;
	}

	//post_Qty
	memset(postcount, 0, sizeof(postcount)) ;
	memcpy(postcount, &saved_buff[2], 14) ;
	POST_COUNT = atoi(postcount) ;
	//file_id
	memset(file_id, 0, sizeof(file_id)) ;
	memcpy(file_id, &saved_buff[17], 5) ;
	send_id = atoi(file_id) + 1;  //从下一个开始计数
	if(send_id < 0)
	{
		send_id = 1;
	}

	close(p_fd) ;

	return 0 ;
}
int SystemPowerOn_ParemerRead() 
{
    //read_powerdown_paramer() ;
#if  0
	printf("\n--读取上一次掉电状态完成--\n") ;
	for(ret=0;ret<22;ret++)
	{
		printf("%c  %d\n", saved_buff[ret], ret) ;
	}
	printf("send_id:%d  POST_count:%d ",send_id, POST_COUNT) ;
	printf("\n----------------------\n") ;
#endif

	return 0 ;
}
int SystemPowerOff_ParemerWrite(int post_status, char* postcount)  
{
	char  save_buff[25] = "0" ;
	char filename_id[5] = "0001" ;     //文件序号
    int  p_fd = 0 ;
	int  ret = 0 ;

	if(post_status == 1){
		save_buff[0] = '1' ;  //mv1
		//如果点击开始 重新清零文件id
		//////send_id =  0 ;
	}
	else if(post_status == 2){
		save_buff[0] = '2' ;  //mv2
	}
	else if(post_status == 3){
		save_buff[0] = '3' ;  //sps
	}
	else if(post_status == 4){
		save_buff[0] = '4' ;  //spe
	}
	else {
		save_buff[0] = '5' ;
	}

    //保存工件数量	
	save_buff[1] = ' ' ;
	memcpy(&save_buff[2], postcount, 15) ;
    //保存文件序号
    sprintf(filename_id, "%04d", send_id) ; //整数转化为字符串
	save_buff[16] = ' ' ;
	memcpy(&save_buff[17], filename_id, 4) ;
    //
	p_fd = open("paramer.txt", O_RDWR | O_CREAT, 666) ;
    if(p_fd < 0)
	{

	}
	ret = write(p_fd, save_buff, sizeof(save_buff)) ;
    lseek(p_fd,0,SEEK_SET) ;
#if  0
	printf("\n------save finish------\n") ;
	for(ret=0;ret<22;ret++)
	{
		printf("%c  %d\n", save_buff[ret], ret) ;
	}
	printf("\n----------------------\n") ;
#endif
	close(p_fd) ;

	return 0 ;
}
//打印日志
void show_log(void)
{
	time_t nowtime;
	struct tm* nowtimeinfo;
	char   nowtime_buf[30] = {0} ;
	char   show_buf[20] = {0} ;
	int   ret_log = 0 ;
	char  log_count[5] = {0} ;
	int   log_fd = 0 ;

	log_fd = open("error_write.txt", O_RDWR | O_APPEND | O_CREAT, 666) ;

	time(&nowtime);
	nowtimeinfo=localtime(&nowtime);

	strftime(nowtime_buf, 30, "\n%Y-%m-%d %H:%M:%S\n", nowtimeinfo);  //24小时制
	memset(show_buf, 0, 20) ;
	memcpy(show_buf, &nowtime_buf[11], 9) ;

	sprintf(log_count, "% 5d", 12345) ;
	strcat(show_buf, log_count) ;
	strcat(show_buf, "\r\n") ;

	ret_log = write(log_fd, show_buf, 20) ;
	system("sync") ;
	close(log_fd) ;

}


