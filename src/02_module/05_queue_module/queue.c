// queue.c
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

static struct task_struct *thread_1, *thread_2;
static atomic_t is_kicked;

DECLARE_WAIT_QUEUE_HEAD(my_queue);

static int thread_1_fct(void* data){
    pr_info("Starting thread 1 of module");
    while(!kthread_should_stop()){
		int status = wait_event_interruptible(my_queue, (atomic_read(&is_kicked) != 0) || kthread_should_stop());
        if(status == -ERESTARTSYS){
			break;
        }else{
            pr_info("Hello from thread_1\n");
        }
        atomic_dec(&is_kicked);
	}
	pr_info ("thread_1 is destroyed\n");
    return 0;
}

static int thread_2_fct(void* data){
    pr_info("Starting thread 2 of module\n");
    while(!kthread_should_stop()){
		ssleep(5);
        pr_info("Hello from thread_2\n");
        atomic_set(&is_kicked, 1);
        wake_up_interruptible(&my_queue);
	}
    return 0;
}

static int __init mod_init(void)
{
    pr_info ("Linux queue module loaded\n");

    atomic_set(&is_kicked, 0);
	thread_1 = kthread_run (thread_1_fct, 0, "my first module thread");
	thread_2 = kthread_run (thread_2_fct, 0, "my second module thread");

	return 0;
}

static void __exit mod_exit(void)
{
    kthread_stop(thread_1);
    kthread_stop(thread_2);

	pr_info("Linux queue module unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Romain Rosset <romain.rosset@hes-so.ch> & Arnaud Yersin <arnaud.yersin@hes-so.ch>");
MODULE_DESCRIPTION("queue module using threads");
MODULE_LICENSE("GPL");
