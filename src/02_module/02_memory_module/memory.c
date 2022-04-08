// memory.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/moduleparam.h>	/* needed for module parameters */

#include <linux/slab.h>		/* needed for dynamic memory allocation */
#include <linux/list.h>		/* needed for linked list processing */
#include <linux/string.h>	/* needed for string handling */

static char* text = "default text";
module_param(text, charp, 0);

static int nbr = 1;
module_param(nbr, int, 0);

typedef struct{
    int n;
    char* text;
    struct list_head node;
}TEXT_ELEMENT;

static LIST_HEAD(list);

static int __init mod_init(void)
{
    int text_length, n;
	pr_info("Linux memory module loaded\n");
    text_length = strlen(text);
    pr_info("create %d elements with text: %s\n", nbr, text);

    for(n = 0; n < nbr; n++){
        TEXT_ELEMENT* element = kzalloc(sizeof(TEXT_ELEMENT), GFP_KERNEL);
        if(element != 0){
            list_add_tail(&element->node, &list);
            element->n = n;
            element->text = kzalloc((text_length+1)*sizeof(char), GFP_KERNEL);
            if(element->text != 0){
                strncpy(element->text, text, text_length);
                element->text[text_length] = '\0';
            }
        }
    }
	return 0;
}

static void __exit mod_exit(void)
{
    TEXT_ELEMENT* element;
    while(!list_empty(&list)){
		element = list_entry(list.next, TEXT_ELEMENT, node);
		list_del(&element->node);
        pr_info("Element number: %d, text: %s\n", element->n, element->text);
        if(element->text != 0){
            kfree(element->text);
        }
		kfree(element);
	}
	pr_info("Linux memory module unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Romain Rosset <romain.rosset@hes-so.ch> & Arnaud Yersin <arnaud.yersin@hes-so.ch>");
MODULE_DESCRIPTION("Memory module");
MODULE_LICENSE("GPL");
