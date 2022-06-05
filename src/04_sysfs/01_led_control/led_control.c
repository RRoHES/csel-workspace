#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <stdio.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <syslog.h>

/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED      "/sys/class/gpio/gpio10"
#define LED           "10"

#define GPIO_SWITCH_PATH   "/sys/class/gpio/gpio"
#define SWITCH_K1        "0"
#define SWITCH_K2        "2"
#define SWITCH_K3        "3"

#define TEXT_SIZE (40)

enum {K1, K2, K3, Timer, TimerK1, TimerK3, NBR_EVENT};

#define MAX_EVENTS (NBR_EVENT)
#define T_PERIOD_START (0.5)
#define T_TIMER_K (0.25)
#define INCREASE_COEFF (1.2)
#define DECREASE_COEFF (0.8)

typedef struct{
    int fd;
    struct epoll_event event;
    char id;
}event_ctrl;

typedef struct{
    int K1;
    int K2;
    int K3;
}fswitch;

static int open_led()
{
    // unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // config pin
    f = open(GPIO_LED "/direction", O_WRONLY);
    write(f, "out", 3);
    close(f);

    // open gpio value attribute
    f = open(GPIO_LED "/value", O_RDWR);
    return f;
}

static int open_switch(const char* gpio_id){

    char sw_path[TEXT_SIZE] = GPIO_SWITCH_PATH;
    char sw_dir_path[TEXT_SIZE];
    char sw_edg_path[TEXT_SIZE];
    char sw_val_path[TEXT_SIZE];

    strcat(sw_path, gpio_id);
    strcpy(sw_dir_path, sw_path);
    strcat(sw_dir_path, "/direction");
    strcpy(sw_edg_path, sw_path);
    strcat(sw_edg_path, "/edge");
    strcpy(sw_val_path, sw_path);
    strcat(sw_val_path, "/value");

    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, gpio_id, strlen(gpio_id));
    close(f);

    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, gpio_id, strlen(gpio_id));
    close(f);

    f = open(sw_dir_path, O_WRONLY);
    write(f, "in", 2);
    close(f);

    f = open(sw_edg_path, O_WRONLY);
    write(f, "rising", 6);
    close(f);

    return open(sw_val_path, O_RDWR);
}

void set_timer(int tfd, double t){
    int t_sec, t_nsec;
    t_sec = (int)t;
    t_nsec = (int)(1e9*(t-(double)t_sec));
    struct itimerspec ts;
    ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = t_sec;
	ts.it_value.tv_nsec = t_nsec;
    timerfd_settime(tfd, 0, &ts, NULL);
}

void start_timer(int epfd, event_ctrl event_timer, double t_timer){
    set_timer(event_timer.fd, t_timer);
    event_timer.event.events = EPOLLIN;
    if(epoll_ctl(epfd, EPOLL_CTL_MOD, event_timer.fd, &event_timer.event))
        printf("epoll_ctl: error\n");
}

void stop_timer(int epfd, event_ctrl event_timer){
    event_timer.event.events = 0;
    if(epoll_ctl(epfd, EPOLL_CTL_MOD, event_timer.fd, &event_timer.event))
        printf("epoll_ctl: error\n");
}

void increase_timing(double* tPeriod){
    *tPeriod = *tPeriod*INCREASE_COEFF;
    syslog(LOG_INFO,"increase timing led");
}

void decrease_timing(double* tPeriod){
    *tPeriod = *tPeriod*DECREASE_COEFF;
    syslog(LOG_INFO,"decrease timing led");
}

int main(void)
{
    event_ctrl event_list[NBR_EVENT], *event;
    char switch_value;
    struct epoll_event events[MAX_EVENTS];
    int stateLED = 0;
    double tPeriod = T_PERIOD_START;

    int led = open_led();

    event_list[K1].fd = open_switch(SWITCH_K1);
    event_list[K2].fd = open_switch(SWITCH_K2);
    event_list[K3].fd = open_switch(SWITCH_K3);
    event_list[Timer].fd = timerfd_create(CLOCK_MONOTONIC, 0);
    event_list[TimerK1].fd = timerfd_create(CLOCK_MONOTONIC, 0);
    event_list[TimerK3].fd = timerfd_create(CLOCK_MONOTONIC, 0);

    int epfd = epoll_create1(0);
    if(epfd == -1)
        printf("epoll_create1: error\n");

    for(int id = 0; id < NBR_EVENT; id++){
        if(id == K1 || id == K2 || id == K3){
            event_list[id].event.events = EPOLLET;
        }else{
            event_list[id].event.events = 0;
        }
        event_list[id].event.data.ptr = &event_list[id];
        event_list[id].id = id;
        if(epoll_ctl(epfd, EPOLL_CTL_ADD, event_list[id].fd, &event_list[id].event))
            printf("epoll_ctl: error, event %d\n", id);
    }
    
    openlog("led_control",LOG_PID,LOG_INFO);
    start_timer(epfd, event_list[Timer], tPeriod/2);

    while (1) {
        int nr = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if(nr == -1){
            printf("epoll_wait: error\n");
        }else{
            for(int i = 0; i < nr; i++){
                event = events[i].data.ptr;
                switch(event->id){
                    case K1:
                        start_timer(epfd, event_list[TimerK1], T_TIMER_K);
                        increase_timing(&tPeriod);
                        break;
                    case K2:
                        tPeriod = T_PERIOD_START;
                        syslog(LOG_INFO,"reset timing led");
                        break;
                    case K3:
                        start_timer(epfd, event_list[TimerK3], T_TIMER_K);
                        decrease_timing(&tPeriod);
                        break;
                    case Timer:
                        set_timer(event_list[Timer].fd, tPeriod/2);
                        stateLED = !stateLED;
                        if(stateLED)
                            pwrite(led, "1", 1, 0);
                        else
                            pwrite(led, "0", 1, 0);
                        break;
                    case TimerK1:
                        pread(event_list[K1].fd, &switch_value, 1, 0);
                        if(switch_value == '1'){
                            set_timer(event_list[TimerK1].fd, T_TIMER_K);
                            increase_timing(&tPeriod);
                        }else{
                            stop_timer(epfd, event_list[TimerK1]);
                        }
                        break;
                    case TimerK3:
                        pread(event_list[K3].fd, &switch_value, 1, 0);
                        if(switch_value == '1'){
                            set_timer(event_list[TimerK3].fd, T_TIMER_K);
                            decrease_timing(&tPeriod);
                        }else{
                            stop_timer(epfd, event_list[TimerK3]);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return 0;
}
