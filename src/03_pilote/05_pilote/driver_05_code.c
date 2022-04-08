/* driver05.c */
#include <linux/cdev.h>   /* needed for char device driver */
#include <linux/device.h> /* needed for sysfs handling */
#include <linux/fs.h>     /* needed for device drivers */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/miscdevice.h>
#include <linux/module.h>          /* needed by all modules */
#include <linux/platform_device.h> /* needed for sysfs handling */
#include <linux/uaccess.h>         /* needed to copy data to/from user */

#define CREATE_ATTR

#ifdef CREATE_ATTR
static char str[100];
static int val;

ssize_t sysfs_show_val(struct device* dev,
                       struct device_attribute* attr,
                       char* buf){
    sprintf(buf, "%d\n", val);
    return strlen(buf);
}
ssize_t sysfs_store_val(struct device* dev,
                        struct device_attribute* attr,
                        const char* buf,
                        size_t count){
    val = simple_strtol(buf, 0, 10);
    return count;
}
DEVICE_ATTR(val, 0664, sysfs_show_val, sysfs_store_val);

ssize_t sysfs_show_str(struct device* dev,
                       struct device_attribute* attr,
                       char* buf){
    sprintf(buf, "%s\n", str);
    return strlen(buf);
}
ssize_t sysfs_store_str(struct device* dev,
                        struct device_attribute* attr,
                        const char* buf,
                        size_t count){
    sscanf(buf, "%s", str);
    return count;
}
DEVICE_ATTR(str, 0664, sysfs_show_str, sysfs_store_str);
#endif

static struct class* sysfs_class;
#ifdef CREATE_ATTR
static struct device* sysfs_device;
#endif

static int __init mod_init(void){
    int status = 0;

    sysfs_class  = class_create(THIS_MODULE, "my_sysfs_class");
    #ifdef CREATE_ATTR
    sysfs_device = device_create(sysfs_class, NULL, 0, NULL, "my_sysfs_device");
    if (status == 0) 
        status = device_create_file(sysfs_device, &dev_attr_val);
    if (status == 0) 
        status = device_create_file(sysfs_device, &dev_attr_str);
    #endif

    pr_info("Linux sysfs module loaded\n");
    return 0;
}

static void __exit mod_exit(void){

    #ifdef CREATE_ATTR
    device_remove_file(sysfs_device, &dev_attr_val);
    device_remove_file(sysfs_device, &dev_attr_str);
    device_destroy(sysfs_class, 0);
    #endif
    class_destroy(sysfs_class);
    
    pr_info("Linux sysfs module unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Romain Rosset <romain.rosset@hes-so.ch> & Arnaud Yersin <arnaud.yersin@hes-so.ch>");
MODULE_DESCRIPTION("Sysfs Module");
MODULE_LICENSE("GPL");