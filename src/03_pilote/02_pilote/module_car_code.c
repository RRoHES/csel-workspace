/* module_car.c */
#include <linux/cdev.h>        /* needed for char device driver */
#include <linux/fs.h>          /* needed for device drivers */
#include <linux/init.h>        /* needed for macros */
#include <linux/kernel.h>      /* needed for debugging */
#include <linux/module.h>      /* needed by all modules */
#include <linux/moduleparam.h> /* needed for module parameters */
#include <linux/uaccess.h>     /* needed to copy data to/from user */

#define BUFFER_SZ 10000

static char s_buffer[BUFFER_SZ];
static dev_t module_car_dev;
static struct cdev module_car_cdev;

static int module_car_open(struct inode* i, struct file* f)
{
    pr_info("module_car : open operation... major:%d, minor:%d\n",
            imajor(i),
            iminor(i));

    if ((f->f_flags & (O_APPEND)) != 0) {
        pr_info("module_car : opened for appending...\n");
    }

    if ((f->f_mode & (FMODE_READ | FMODE_WRITE)) != 0) {
        pr_info("module_car : opened for reading & writing...\n");
    } else if ((f->f_mode & FMODE_READ) != 0) {
        pr_info("module_car : opened for reading...\n");
    } else if ((f->f_mode & FMODE_WRITE) != 0) {
        pr_info("module_car : opened for writing...\n");
    }

    return 0;
}

static int module_car_release(struct inode* i, struct file* f)
{
    pr_info("module_car: release operation...\n");

    return 0;
}

static ssize_t module_car_read(struct file* f,
                             char __user* buf,
                             size_t count,
                             loff_t* off)
{
    // compute remaining bytes to copy, update count and pointers
    ssize_t remaining = BUFFER_SZ - (ssize_t)(*off);
    char* ptr         = s_buffer + *off;
    if (count > remaining) count = remaining;
    *off += count;

    // copy required number of bytes
    if (copy_to_user(buf, ptr, count) != 0) count = -EFAULT;

    pr_info("module_car: read operation... read=%ld\n", count);

    return count;
}

static ssize_t module_car_write(struct file* f,
                              const char __user* buf,
                              size_t count,
                              loff_t* off)
{
    // compute remaining space in buffer and update pointers
    ssize_t remaining = BUFFER_SZ - (ssize_t)(*off);

    pr_info("module_car: at%ld\n", (unsigned long)(*off));

    // check if still remaining space to store additional bytes
    if (count >= remaining) count = -EIO;

    // store additional bytes into internal buffer
    if (count > 0) {
        char* ptr = s_buffer + *off;
        *off += count;
        ptr[count] = 0;  // make sure string is null terminated
        if (copy_from_user(ptr, buf, count)) count = -EFAULT;
    }

    pr_info("module_car: write operation... written=%ld\n", count);

    return count;
}

static struct file_operations module_car_fops = {
    .owner   = THIS_MODULE,
    .open    = module_car_open,
    .read    = module_car_read,
    .write   = module_car_write,
    .release = module_car_release,
};

static int __init module_car_init(void)
{
    int status = alloc_chrdev_region(&module_car_dev, 0, 1, "mymodule");
    if (status == 0) {
        cdev_init(&module_car_cdev, &module_car_fops);
        module_car_cdev.owner = THIS_MODULE;
        status              = cdev_add(&module_car_cdev, module_car_dev, 1);
    }

    pr_info("Linux module module_car loaded\n");
    return 0;
}

static void __exit module_car_exit(void)
{
    cdev_del(&module_car_cdev);
    unregister_chrdev_region(module_car_dev, 1);

    pr_info("Linux module module_car unloaded\n");
}

module_init(module_car_init);
module_exit(module_car_exit);

MODULE_AUTHOR ("Romain Rosset <romain.rosset@hes-so.ch> & Arnaud Yersin <arnaud.yersin@hes-so.ch>");
MODULE_DESCRIPTION("Module module_car");
MODULE_LICENSE("GPL");
