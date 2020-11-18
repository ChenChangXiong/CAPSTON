#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include  <linux/fs.h>
#include  <linux/miscdevice.h>
#include  <asm/io.h>
#include <linux/timer.h>

#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include  <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
/*
 *   ioctl  MV1--(1,0)  SPS--(1,1)  SPE--(0,1)   MV3--(1,1)
 *          start       palse       stop_palse   stop
 */
#define      STATUS_MV1		_IO('l',  0)
#define      STATUS_SPS		_IO('l',  1)
#define      STATUS_SPE		_IOW('l',  2,  long)
#define      STATUS_MV3		_IOW('l',  3,  long)
#define      BEEP_ON		_IOW('l',  4,  long)
#define      BEEP_OFF		_IOW('l',  5,  long)
#define      DISCONNECT		_IOW('l',  6,  long)
#define      ON_CONNECT		_IOW('l',  7,  long)

#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

//#define   KERNEL_POST_TIME               //for kernel timer post
 
/*
-----           ----------            ---------
    |           |        |           |
    |__/\/\_____|        |__/\/\_____|
*/
static  struct   timer_list    input_timer_fiter;       //fiter  timer
#ifdef  KERNEL_POST_TIME
static  struct   timer_list    post_timer ;              //post  timer
#endif
static  struct fasync_struct*  input_post_fasync = NULL;//fasync 
struct misc_leds{
      unsigned int gpio;
	  char* name;
	  int value; 
	  int irq;
};

static struct misc_leds _gpios[]={
         	{GPIO_TO_PIN(3,21),"DI_IN",0,0},
         	//{0,"xxx",0,0},
			{GPIO_TO_PIN(3,17),"Relay1",0,0},
			{GPIO_TO_PIN(3,18),"Relay2",1,0},
};
//irq handdler
static  irqreturn_t   input_irq_handler(int  irq, void *  data)
{
	if(irq ==147)
	{
		mod_timer(&input_timer_fiter, jiffies + HZ/4 + HZ/10) ;  //10ms fiteref
	}
	else{
		return  IRQ_NONE;
	}
    //mod_timer(&input_timer_fiter, jiffies + HZ/2) ;  //10ms fiter
	return  IRQ_HANDLED;
}
//fiter timer handler
static  void   input_fiter_timer_handler(unsigned long  data)
{
	if(gpio_get_value(_gpios[0].gpio)!=0)
	{
			kill_fasync(&input_post_fasync,  SIGIO,  POLL_IN);
	}
}
#ifdef  KERNEL_POST_TIME
static  void   post_timer_handler(unsigned long  data)
{
	    p_time++;
		mod_timer(&post_timer, jiffies + HZ/50) ;  //10ms fiter
}
#endif
//ioctl
long  status_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case  STATUS_MV1: {
			gpio_set_value(_gpios[1].gpio,1) ;			
			//gpio_set_value(_gpios[2].gpio,0) ;		
			gpio_set_value(_gpios[2].gpio,1) ;						
			break;
		}
		case  STATUS_SPS: {
			gpio_set_value(_gpios[1].gpio,1) ;			
			//gpio_set_value(_gpios[2].gpio,1) ;	
			gpio_set_value(_gpios[2].gpio,0) ;	
			break;
		}
		case  STATUS_SPE: {
			gpio_set_value(_gpios[1].gpio,0) ;	   
			gpio_set_value(_gpios[2].gpio,1) ;		
			break;
		}	
		case  STATUS_MV3: {
			gpio_set_value(_gpios[1].gpio,0) ;			
			gpio_set_value(_gpios[2].gpio,0) ;
			break;
		}
		case  BEEP_ON: {
			gpio_set_value(GPIO_TO_PIN(3,15) , 1) ;		
			break;
		}
		case  BEEP_OFF: {
			gpio_set_value(GPIO_TO_PIN(3,15), 0) ;		  
			break;			
		}
		case  DISCONNECT: {
			gpio_set_value(GPIO_TO_PIN(3,16) , 1) ;		
			break ;
		}
		case  ON_CONNECT: {
			gpio_set_value(GPIO_TO_PIN(3,16), 0) ;	
			break;
		}
		default :  break;
	}

	return  0;
}
//fasync
static   int   post_fasync(int  fd, struct file*  filp, int  on)
{
	return  fasync_helper(fd,  filp,  on,  &input_post_fasync);
}
//fileoperations
static struct file_operations postfops={
   .owner = THIS_MODULE,
   	.fasync 	=	post_fasync,         //实现io信号异步通信
   	.unlocked_ioctl =  status_ioctl,     //实现ioctl接口
};
static struct miscdevice  input_post_misc ={
     .minor = 255 ,
	 	.name = "post_misc",
	 	.fops = &postfops,
};
//platform
static const char*  p_name ;
static  int   post_palt_prob(struct platform_device *  pdev)
{
	int ret = 0  ; 
	struct device_node  *node = pdev->dev.of_node;
#if  0
	//printk("paramer:pdev->name:%s\r\n", pdev->name);
    //探测资源
    ret = of_property_read_string(node, "di_name", &p_name);
    if(ret < 0) {
        printk("error of_property_read_string with \"compatible\" \n");
        return -EINVAL;
    }

#endif
    ret = of_property_read_string(node, "di_name", &p_name);
    if(ret < 0) {
        printk("error of_property_read_string with \"compatible\" \n");
        return -EINVAL;
    }
	_gpios[0].name = (char*)p_name ; 
	printk("lable: %s\n",_gpios[0].name) ;
	//get irq
    ret = of_irq_get(node,0) ;
    if(ret < 0) {
        printk("error of_get_irq\n");
        return -EINVAL;
    }
	printk("irq:%d \n",ret) ;

    _gpios[0].irq = ret ;
	ret = request_irq(_gpios[0].irq,  input_irq_handler, IRQ_TYPE_EDGE_RISING, _gpios[0].name,  (void*)&_gpios[0]); //IRQF_SHARED  | IRQF_TRIGGER_FALLING
	
	return 0;
}

static  int   post_plat_remove(struct platform_device *  pdev)
{
    free_irq(_gpios[0].irq, (void*)&_gpios[0]) ;
	return 0;
}
static struct  of_device_id  post_device_id[] = {
			{.compatible = "post,irq_dts"} ,
			{.compatible = "xxx_irq"},
} ;
static  struct platform_driver  post_plat = {
		.probe = post_palt_prob,
		.remove = post_plat_remove,
		.driver = {
				.name = "cmi_at153_driver",
				.owner = THIS_MODULE,
				.of_match_table = post_device_id,
			},
} ;
static  int  __init   key_module_init(void)
{
	int ret = 0 ;
	gpio_request(_gpios[0].gpio, _gpios[0].name) ;
	_gpios[0].irq = gpio_to_irq(_gpios[0].gpio);
	
	ret = request_irq(_gpios[0].irq,  input_irq_handler, 
			IRQF_SHARED  | IRQF_TRIGGER_RISING , 
			_gpios[0].name,  (void*)&_gpios[0]); //IRQF_SHARED | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	
    printk("---irq: %d %d---\n",ret, _gpios[0].irq) ;
	
	gpio_request(_gpios[1].gpio, _gpios[1].name) ;
    gpio_direction_output(_gpios[1].gpio, _gpios[1].value) ;
	gpio_request(_gpios[2].gpio, _gpios[2].name) ;
    gpio_direction_output(_gpios[2].gpio, _gpios[2].value) ;
	//beep
	gpio_request(GPIO_TO_PIN(3,15), "beep contrl") ;
    gpio_direction_output(GPIO_TO_PIN(3,15), 0) ;
	//disconnect
	gpio_request(GPIO_TO_PIN(3,16), "disconnect") ;
    gpio_direction_output(GPIO_TO_PIN(3,16), 0) ;
	//fiter timer
	setup_timer(&input_timer_fiter,  input_fiter_timer_handler,  (unsigned long)"led_timer");
	mod_timer(&input_timer_fiter, jiffies + HZ/1) ;  //start
    //possed timer
#ifdef  KERNEL_POST_TIME
	setup_timer(&post_timer,  post_timer_handler,  (unsigned long)"led_timer");
    mod_timer(&post_timer, jiffies + HZ/1) ;  
#endif
	misc_register(&input_post_misc) ;
	//////////////platform_driver_register(&post_plat) ;
	return  0;
}

static  void  __exit   key_module_exit(void)
{
    printk("---rmmod success---\n") ;  
    gpio_free(_gpios[0].gpio) ;
    gpio_free(_gpios[1].gpio) ;
    gpio_free(_gpios[2].gpio) ;  
	
	gpio_free(GPIO_TO_PIN(3,15)) ;  //beeb
	gpio_free(GPIO_TO_PIN(3,16)) ;  //disconnect 
	
	misc_deregister(&input_post_misc) ;
	del_timer(&input_timer_fiter);
#ifdef  KERNEL_POST_TIME
    del_timer(&post_timer);
#endif
	free_irq(_gpios[0].irq, (void*)&_gpios[0]) ;
    /////////platform_driver_unregister(&post_plat) ;
}

module_init(key_module_init);
module_exit(key_module_exit);

MODULE_LICENSE("GPL");


















