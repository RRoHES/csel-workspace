/* interrupt.c */
#include <linux/cdev.h>      /* needed for char device driver */
#include <linux/fs.h>        /* needed for device drivers */
#include <linux/gpio.h>      /* needed for i/o handling */
#include <linux/init.h>      /* needed for macros */
#include <linux/interrupt.h> /* needed for interrupt handling */
#include <linux/io.h>        /* needed for mmio handling */
#include <linux/ioport.h>    /* needed for memory region handling */
#include <linux/kernel.h>    /* needed for debugging */
#include <linux/miscdevice.h>
#include <linux/module.h>  /* needed by all modules */
#include <linux/poll.h>    /* needed for polling handling */
#include <linux/sched.h>   /* needed for scheduling constants */
#include <linux/uaccess.h> /* needed to copy data to/from user */
#include <linux/wait.h>    /* needed for wating */

#define K1 0
#define K2 2
#define K3 3

static char* k1 = "gpio_a.0-k1";
static char* k2 = "gpio_a.2-k2";
static char* k3 = "gpio_a.3-k3";

static atomic_t nb_of_interrupts;
DECLARE_WAIT_QUEUE_HEAD(queue);

irqreturn_t gpio_isr(int irq, void* handle)
{
    atomic_inc(&nb_of_interrupts);
    wake_up_interruptible(&queue);

    pr_info("interrupt %s raised...\n", (char*)handle);

    return IRQ_HANDLED;
}

static ssize_t interrupt_read(struct file* f,
                             char __user* buf,
                             size_t sz,
                             loff_t* off)
{
    return 0;
}

static unsigned int interrupt_poll(struct file* f, poll_table* wait)
{
    unsigned mask = 0;
    poll_wait(f, &queue, wait);
    if (atomic_read(&nb_of_interrupts) != 0) {
        mask |= POLLIN | POLLRDNORM; /* read operation */
        /* mask |= POLLOUT | POLLWRNORM;   write operation */
        atomic_dec(&nb_of_interrupts);
        pr_info("polling thread waked-up...\n");
    }
    return mask;
}

static struct file_operations interrupt_fops = {
    .owner = THIS_MODULE,
    .read  = interrupt_read,
    .poll  = interrupt_poll,
};

struct miscdevice misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .fops  = &interrupt_fops,
    .name  = "interruptmodule",
    .mode  = 0777,
};

static int __init interrupt_init(void)
{
    int status = 0;

    atomic_set(&nb_of_interrupts, 0);

    status = misc_register(&misc_device);

    // install k1
    if (status == 0)
        status = devm_request_irq(misc_device.this_device,
                                  gpio_to_irq(K1),
                                  gpio_isr,
                                  IRQF_TRIGGER_FALLING | IRQF_SHARED,
                                  k1,
                                  k1);

    // install k2
    if (status == 0)
        status = devm_request_irq(misc_device.this_device,
                                  gpio_to_irq(K2),
                                  gpio_isr,
                                  IRQF_TRIGGER_RISING | IRQF_SHARED,
                                  k2,
                                  k2);

    // install k3
    if (status == 0)
        status = devm_request_irq(
            misc_device.this_device,
            gpio_to_irq(K3),
            gpio_isr,
            IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_SHARED,
            k3,
            k3);

    pr_info("Linux module interrupt loaded(status=%d)\n", status);
    return status;
}

static void __exit interrupt_exit(void)
{
    misc_deregister(&misc_device);
    pr_info("Linux module interrupt unloaded\n");
}

module_init(interrupt_init);
module_exit(interrupt_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module interrupt");
MODULE_LICENSE("GPL");
