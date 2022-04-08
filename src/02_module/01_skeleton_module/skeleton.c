// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/moduleparam.h>	/* needed for module parameters */

static char* text = "default text";
module_param(text, charp, 0);

static int nbr = 1;
module_param(nbr, int, 0);

static int __init skeleton_init(void)
{
	pr_info("Linux module skeleton loaded\n");
	pr_info("text is: %s\nnumber is: %d\n", text, nbr);
	return 0;
}

static void __exit skeleton_exit(void)
{
	pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Romain Rosset <romain.rosset@hes-so.ch> & Arnaud Yersin <arnaud.yersin@hes-so.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
