// iomem.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging
#include <linux/types.h>    // needed for types uintxx_t

#include <linux/moduleparam.h>	/* needed for module parameters */

#include <linux/slab.h>		/* needed for dynamic memory allocation */
#include <linux/list.h>		/* needed for linked list processing */
#include <linux/string.h>	/* needed for string handling */

#include <linux/ioport.h>	/* needed for memory region handling */
#include <linux/io.h>		/* needed for mmio handling */

static struct resource* res_chipID = 0;
static struct resource* res_temp = 0;
static struct resource* res_MACadd = 0;

static int __init mod_init(void)
{
    unsigned char* reg_chipID = 0;
    uint32_t chipID[4] = {[0]=0,};

    unsigned char* reg_temp = 0;
    int64_t temp = 0;

    unsigned char* reg_MACadd = 0;
    uint32_t macAddr[2] = {[0]=0,};

	pr_info("Linux I/O memory module loaded\n");

    // chip ID
    res_chipID = request_mem_region(0x01c14200, 0x0010, "allwiner h5 sid");
    if(res_chipID == 0)
        pr_info("Warning: region of chip ID is already used\n");

    reg_chipID = ioremap(0x01c14200, 0x0010);
    if(reg_chipID == 0){
		pr_info("Error: mapping of chip ID region fail\n");
	}else{
        chipID[0] = ioread32(reg_chipID);
	    chipID[1] = ioread32(reg_chipID+0x004);
	    chipID[2] = ioread32(reg_chipID+0x008);
	    chipID[3] = ioread32(reg_chipID+0x00c);
        pr_info("chip ID is: %08x'%08x'%08x'%08x\n", chipID[0], chipID[1], chipID[2], chipID[3]);
    }

    // temperature
    res_temp = request_mem_region(0x01c25080, 0x0004, "allwiner h5 ths");
    if(res_temp == 0)
        pr_info("Warning: region of temperature is already used\n");

    reg_temp = ioremap(0x01c25080, 0x0004);
    if(reg_temp == 0){
		pr_info("Error: mapping of temperature region fail\n");
	}else{
        temp = (int64_t)ioread32(reg_temp);
        temp = -1191*temp/10 + 223000;
        pr_info("temperature is: %ld\n", (long int)temp);
    }

    // MAC address
    res_MACadd = request_mem_region(0x01c30050, 0x0008, "allwiner h5 emac");
    if(res_MACadd == 0)
        pr_info("Warning: region of MAC address is already used\n");

    reg_MACadd = ioremap(0x01c30050, 0x0008);
    if(reg_MACadd == 0){
		pr_info("Error: mapping of MAC address region fail\n");
	}else{
        macAddr[0] = ioread32(reg_MACadd);
        macAddr[1] = ioread32(reg_MACadd+0x004);
        pr_info("MAC address is: %02x:%02x:%02x:%02x:%02x:%02x\n",
			(macAddr[1]>> 0) & 0xff,
			(macAddr[1]>> 8) & 0xff,
			(macAddr[1]>>16) & 0xff,
			(macAddr[1]>>24) & 0xff,
			(macAddr[0]>> 0) & 0xff,
			(macAddr[0]>> 8) & 0xff);
    }

    iounmap(reg_chipID);
	iounmap(reg_temp);
	iounmap(reg_MACadd);

	return 0;
}

static void __exit mod_exit(void)
{
    if(res_chipID != 0) 
        release_mem_region(0x01c14200, 0x0010);
    if(res_temp != 0) 
        release_mem_region(0x01c25080, 0x0004);
    if(res_MACadd != 0) 
        release_mem_region(0x01c30050, 0x0008);

	pr_info("Linux I/O memory module unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Romain Rosset <romain.rosset@hes-so.ch> & Arnaud Yersin <arnaud.yersin@hes-so.ch>");
MODULE_DESCRIPTION("I/O memory module");
MODULE_LICENSE("GPL");
