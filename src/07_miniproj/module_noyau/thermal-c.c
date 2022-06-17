// thermal_control.c
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

#include <linux/ioport.h>	/* needed for memory region handling */
#include <linux/io.h>		/* needed for mmio handling */

// Libs proposed for mini-project
#include <linux/thermal.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#define MODE_AUTO                   "auto"
#define MODE_MANUAL                 "man"
#define THERMAL_ZONE_NAME           "cpu-thermal"
#define TEMP_REFRESH_RATE_MS        500
#define AUTO_SPEED_REFRESH_RATE_MS  1000
#define LED_SPEED_REFRESH_RATE_MS   500
#define LED_GPIO                    10
#define FREQ_MIN                    99
#define FREQ_MAX                    99

// Speed tables
typedef enum                 {SPEED_MIN, SPEED_1, SPEED_2, SPEED_MAX} speed_value; // 4 thresholds for frequency
const int speed_table[]    = {2,         5,       10,      20};                    // Hz
const int temp_threshold[] = {35000,     40000,   45000}; // to adjust with final value 35k, 40k and 45k

// Initial values
speed_value fan_speed   = SPEED_MIN;
int temp_value          = temp_threshold[SPEED_MIN];
int freq                = speed_table[SPEED_MIN];
int period_interval     = 1000/speed_table[SPEED_MIN];
static char mode[20]    = MODE_AUTO;

// Variables
static struct task_struct *thread_1, *thread_2;
struct timer_list timerTemp, timerFreq;
struct thermal_zone_device* thermal_zone;
static struct class* sysfs_class;
static struct device* sysfs_device;

// Sysfs functions
// Showing
ssize_t sysfs_show_freq(struct device* dev,
                       struct device_attribute* attr,
                       char* buf){
    sprintf(buf, "%d\n", freq);
    return strlen(buf);
}
ssize_t sysfs_show_mode(struct device* dev,
                       struct device_attribute* attr,
                       char* buf){
    sprintf(buf, "%s\n", mode);
    return strlen(buf);
}
ssize_t sysfs_show_temp(struct device* dev,
                       struct device_attribute* attr,
                       char* buf){
    sprintf(buf, "%d\n", temp_value);
    return strlen(buf);
}

// Storage
ssize_t sysfs_store_freq(struct device* dev,
                        struct device_attribute* attr,
                        const char* buf,
                        size_t count){
    freq = simple_strtol(buf, 0, 10);
    if(freq < FREQ_MIN)
        freq = 0;
    else if(freq > FREQ_MAX)
        freq = FREQ_MAX;
    return count;
}
ssize_t sysfs_store_mode(struct device* dev,
                        struct device_attribute* attr,
                        const char* buf,
                        size_t count){
    sscanf(buf, "%s", mode);
    return count;
}

// Linking callbacks with sysfs files
DEVICE_ATTR(freq, 0664, sysfs_show_freq, sysfs_store_freq);
DEVICE_ATTR(temp, 0444, sysfs_show_temp, NULL);
DEVICE_ATTR(mode, 0664, sysfs_show_mode, sysfs_store_mode);

// Take measurement of the temperature and reset timer
void temp_measurement_handler(struct timer_list *lst)
{
    thermal_zone_get_temp(thermal_zone, &temp_value);
    mod_timer(&timerTemp, jiffies + msecs_to_jiffies(TEMP_REFRESH_RATE_MS));
}

// Thread to update auto cpu temperature
static int thread_temp_cpu(void* data)
{
    bool speed_determined;
    pr_info("Starting thread for auto temp");

    while(!kthread_should_stop()){

        //pr_info("temperature is: %d.%03d\n", temp_value/1000, temp_value%1000);   

        // Auto mode determine frequency from temperature
        if(strcmp(mode,MODE_AUTO)==0)
        {
            fan_speed = SPEED_MIN;
            speed_determined = false;

            // Test all thresholds in temp_threshold table
            while(speed_determined == false)
            {
                if(temp_value <= temp_threshold[fan_speed])
                    speed_determined = true;
                else
                {
                    if(++fan_speed == SPEED_MAX)
                        speed_determined = true;
                }
            }

            //pr_info("speed is: %d\n", fan_speed);
        }
        else // Manual control, nothing to do
        {
            //pr_info("speed is: manual\n");
        }
        
        msleep(AUTO_SPEED_REFRESH_RATE_MS); // update auto fan speed each second
    }

	pr_info ("thread auto temp is destroyed\n");
    return 0;
}

// Toggle the led and reset the timer
void led_toggle_handler(struct timer_list *lst)
{
    static bool ledStatus = false;

    gpio_set_value(LED_GPIO, ledStatus ^= true);

    mod_timer(&timerFreq, jiffies + msecs_to_jiffies(period_interval/2));
}

// Thread updating the period interval for the LED
static int thread_led_speed(void* data)
{
    pr_info("Starting thread led speed\n");

    while(!kthread_should_stop()){

        if(strcmp(mode,MODE_AUTO)==0)
        {
            freq = speed_table[fan_speed];
            period_interval = 1000/freq;
        }
        else if(strcmp(mode,MODE_MANUAL)==0)
            period_interval = 1000/freq;
        else
        {
            period_interval = 1000;
	        pr_info ("Mode unknown\n");
        }
        msleep(LED_SPEED_REFRESH_RATE_MS); // update fan speed each second
	}

	pr_info ("thread led speed is destroyed\n");
    return 0;
}

// Initialize the module
static int __init mod_init(void)
{
    int status = 0;
    pr_info ("Linux thermal control module loaded\n");

    // Create file structure for communication
    sysfs_class  = class_create(THIS_MODULE, "thermal_control_class");
    sysfs_device = device_create(sysfs_class, NULL, 0, NULL, "thermal_control_device");
    status = device_create_file(sysfs_device, &dev_attr_freq);
    if (status == 0) 
        status = device_create_file(sysfs_device, &dev_attr_temp);
    if (status == 0) 
        status = device_create_file(sysfs_device, &dev_attr_mode);
    if (status == 0) 
        pr_info ("Created sysfs structure\n");
    else
    {
        pr_err ("Failed to create sysfs strcture\n");
        return -1;
    }

    // Configure status LED
    if (gpio_request(LED_GPIO, "ledStatus") != 0)
    {
        pr_err("Cannot request LED gpio\n");
        return -1;
    }
    if (gpio_direction_output(LED_GPIO, 0) != 0)
    {
        pr_err("Cannot set LED direction\n");
        return -1;
    }

    // Configure thermal measure
    thermal_zone = thermal_zone_get_zone_by_name(THERMAL_ZONE_NAME);
    if (thermal_zone)
    {
        pr_err("Cannot get thermal zone! But may still work...\n");
    }

    // Launch the timers
    timer_setup(&timerFreq, led_toggle_handler, 0);
    mod_timer(&timerFreq, jiffies + msecs_to_jiffies(period_interval/2));

    timer_setup(&timerTemp, temp_measurement_handler, 0);
    mod_timer(&timerTemp,jiffies + msecs_to_jiffies(TEMP_REFRESH_RATE_MS));

    // Start threads
	thread_1 = kthread_run (thread_temp_cpu, 0, "CPU temperature thread");
	thread_2 = kthread_run (thread_led_speed, 0, "Led speed thread");

	return 0;
}

// Close the module
static void __exit mod_exit(void)
{
    kthread_stop(thread_1);
    kthread_stop(thread_2);

    del_timer(&timerTemp);

    del_timer(&timerFreq);
    gpio_free(LED_GPIO);
    
    device_remove_file(sysfs_device, &dev_attr_freq);
    device_remove_file(sysfs_device, &dev_attr_temp);
    device_remove_file(sysfs_device, &dev_attr_mode);
    device_destroy(sysfs_class, 0);
    class_destroy(sysfs_class);

	pr_info("Linux thermal control module unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Romain Rosset <romain.rosset@hes-so.ch> & Arnaud Yersin <arnaud.yersin@hes-so.ch>");
MODULE_DESCRIPTION("thermal control module");
MODULE_LICENSE("GPL");
