// thread.c
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

static struct task_struct* my_thread;

static int thread_fct(void* data){
    pr_info("Starting thread of module");
    while(!kthread_should_stop()){
		ssleep(5);
		pr_info("Hello from the thread of module\n");
	}
    return 0;
}

static int __init mod_init(void)
{
    pr_info ("Linux thread module loaded\n");

	my_thread = kthread_run(thread_fct, 0, "my module thread");

	return 0;
}

static void __exit mod_exit(void)
{
    kthread_stop(my_thread);

	pr_info("Linux thread module unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Romain Rosset <romain.rosset@hes-so.ch> & Arnaud Yersin <arnaud.yersin@hes-so.ch>");
MODULE_DESCRIPTION("thread module");
MODULE_LICENSE("GPL");
