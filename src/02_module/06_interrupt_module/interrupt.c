// interrupt.c
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

#include <linux/kthread.h>	/* needed for kernel thread management */
#include <linux/delay.h>	/* needed for delay fonctions */
#include <linux/wait.h>		/* needed for waitqueues handling */

#include <linux/interrupt.h>	/* needed for interrupt handling */
#include <linux/gpio.h>			/* needed for i/o handling */

#define K1	0
#define K2	2
#define K3	3

static char* k1="gpio_a.0-k1";
static char* k2="gpio_a.2-k2";
static char* k3="gpio_a.3-k3";

irqreturn_t gpio_isr(int irq, void* handle){
	pr_info ("interrupt %s raised...\n", (char*)handle);
	return IRQ_HANDLED;
}

static int __init mod_init(void)
{
	int status = 0;

	if(status == 0) 
		status = gpio_request(K1, "k1");
	if(status == 0)
		status = request_irq(gpio_to_irq(K1), gpio_isr,
			IRQF_TRIGGER_FALLING | IRQF_SHARED, k1, k1);

	if(status == 0) 
		status = gpio_request(K2, "k2");
	if(status == 0)
		status = request_irq(gpio_to_irq(K2), gpio_isr,
			IRQF_TRIGGER_FALLING | IRQF_SHARED, k2, k2);

	if(status == 0) 
		status = gpio_request(K3, "k3");
	if(status == 0)
		status = request_irq(gpio_to_irq(K3), gpio_isr,
			IRQF_TRIGGER_FALLING | IRQF_SHARED, k3, k3);

	pr_info ("Linux interrupt module loaded\n");
	return status;
}

static void __exit mod_exit(void)
{
	gpio_free(K1);
	free_irq(gpio_to_irq(K1), k1);

	gpio_free(K2);
	free_irq(gpio_to_irq(K2), k2);

	gpio_free(K3);
	free_irq(gpio_to_irq(K3), k3);

	pr_info ("Linux interrupt module unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Romain Rosset <romain.rosset@hes-so.ch> & Arnaud Yersin <arnaud.yersin@hes-so.ch>");
MODULE_DESCRIPTION("interrupt module");
MODULE_LICENSE("GPL");
