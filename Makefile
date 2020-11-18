

FILE := module_di_post
obj-m += $(FILE).o  

KDIR      := /home/aplex/work/CMI_AT153/kernel4.4.12
targetdir := ~/

#
#   ------------------  module makefile---------------
#

all:
	@rm     $(FILE).c 
	@cp     $(targetdir)/$(FILE).c  ./
	@echo   "---copy finish---"
	@chmod  666  $(FILE).c

	make -C $(KDIR)  M=$(PWD)  modules		

	@rm  *.order  *.sym*  *.mod*  .*.cmd  *.o .tmp*  -rf
	@echo  "======copy to windows======"
	@cp  *.ko  $(targetdir)/
#	@clear
#   @ls
clean:
	@rm  *.order  *.sym*  *.mod*  .*.cmd  *.o .tmp*  -rf

chmod:
	@chmod  666  $(FILE).c

bsp:
	@cp ./*.ko    /mnt/share/

#
#  ----------------- application makefile--------------
#
app:
	#	arm-linux-gnueabihf-gcc     -c     cmi_at155_application.c   
	#	arm-linux-gnueabihf-gcc     -c     do_passwd.c   
	#	arm-linux-gnueabihf-gcc     -c     post_to_service.c   
	#	arm-linux-gnueabihf-gcc     -c     client_ftp.c   
	#	arm-linux-gnueabihf-gcc     -c     set_ip_adress.c   
	arm-linux-gnueabihf-gcc   cmi_at155_application.c    do_passwd.c   post_to_service.c   client_ftp.c  set_ip_adress.c -o  DI_POST  -pthread
	@cp          DI_POST                    $(targetdir)/
	sync
	#rm           *.o
	@echo        "---app has finished---"

service:
	cp        $(targetdir)/service.c    ./  -f 
	rm         service*  -f
	cp        $(targetdir)/service.c    ./ 
	chmod     666 *.c
	arm-linux-gnueabihf-gcc    service.c -o   service_connect  -pthread
	cp        service_connect          $(targetdir)/

client:
	arm-linux-gnueabihf-gcc    client.c  -o   client_connect   -pthread
	@cp        client_connect         $(targetdir)/
	@echo     "---client app has finished---"  

#
#  -------------------- uart application---------------
#
uart:
	rm    uart*
	cp    /mnt/share/work_demo/CMI_AT153/demo/project/uart_test.c  ./
	arm-linux-gnueabihf-gcc  uart_test.c  -o   uart_test   
	cp    uart_test  /mnt/share/work_demo/CMI_AT153/demo/project/
#rm:
#   
ftp:
	cp        $(targetdir)/client_ftp.c    ./  -f 
	rm         client_ftp*   -f
	sync
	cp        $(targetdir)/client_ftp.c    ./ 
	sync
	arm-linux-gnueabihf-gcc      client_ftp.c   -o   client_ftp
	#gcc      client_ftp.c   -o   client_ftp
	@cp          client_ftp                    $(targetdir)/
	sync
	@echo     "******copy finish******"
cp:
	cp   $2    $(targetdir)
