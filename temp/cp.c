#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	int ret = 0 ;

	while(1)
	{
		ret = system("sudo mount -t cifs //192.168.4.188/old_files /mnt/ -o username=abc,password=123") ;
		printf("---mount resualt = %d \n", ret) ;
		//sleep(1) ;
		ret = system("sudo umount /mnt/") ;
		printf("---umount resualt = %d \n", ret) ;
		//sleep(1) ;
	}
	return 0 ;
}
