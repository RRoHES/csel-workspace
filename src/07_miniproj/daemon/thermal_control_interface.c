#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <syslog.h>

#include "ssd1306.h"

#define MODE_PATH "/sys/class/thermal_control_class/thermal_control_device/mode"
#define FREQ_PATH "/sys/class/thermal_control_class/thermal_control_device/freq"
#define TEMP_PATH "/sys/class/thermal_control_class/thermal_control_device/temp"

#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED      "/sys/class/gpio/gpio362"
#define LED           "362"

#define GPIO_SWITCH_PATH   "/sys/class/gpio/gpio"
#define SWITCH_K1        "0"
#define SWITCH_K2        "2"
#define SWITCH_K3        "3"

#define TEXT_SIZE (40)
#define MODE_SIZE (6)

#define STR_LENGTH_OLED (17)

#define PERIOD_OLED (0.2)
#define TIMING_LED (0.1)

enum {K1, K2, K3, TIMER_OLED, TIMER_LED, SOCKET, SOCKET_CONN, NBR_EVENT};

typedef enum {MODE_MANU, MODE_AUTO, MODE_ERR}MODE;

#define MAX_CONN (16)
#define DEFAULT_PORT (8080)
#define BUF_SIZE (40)


typedef struct event_ctrl event_ctrl;
struct event_ctrl{
    int fd;
    struct epoll_event event;
    char id;
    void (*fn_ptr)(int, event_ctrl*);
};

// GPIO -----------------------------------------------------------------------

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

void power_led_init(){
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
    pwrite(f, "0", 1, 0);
    close(f);
}

void power_led_on(){
    int f = open(GPIO_LED "/value", O_RDWR);
    pwrite(f, "1", 1, 0);
    close(f);
}

void power_led_off(){
    int f = open(GPIO_LED "/value", O_RDWR);
    pwrite(f, "0", 1, 0);
    close(f);
}

// Timers ---------------------------------------------------------------------

void set_timer(int tfd, double t_timer){
    int t_sec, t_nsec;
    t_sec = (int)t_timer;
    t_nsec = (int)(1e9*(t_timer-(double)t_sec));
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
        syslog(LOG_INFO,"epoll_ctl: error start_timer\n");
}

void stop_timer(int epfd, event_ctrl event_timer){
    event_timer.event.events = 0;
    if(epoll_ctl(epfd, EPOLL_CTL_MOD, event_timer.fd, &event_timer.event))
        syslog(LOG_INFO,"epoll_ctl: error stop_timer\n");
}

// OLED display ---------------------------------------------------------------

void oled_init(void){
    ssd1306_init();
    ssd1306_set_position (0,0);
    ssd1306_puts("CSEL Mini-projet");
    ssd1306_set_position (0,1);
    ssd1306_puts("17.06.2022      ");
    ssd1306_set_position (0,2);
    ssd1306_puts("Romain Rosset   ");
    ssd1306_set_position (0,3);
    ssd1306_puts("Arnaud Yersin   ");
    ssd1306_set_position (0,4);
    ssd1306_puts("----------------");
}

void refresh_oled(MODE mode, int temp, int freq){
    char mode_str[STR_LENGTH_OLED];
    char temp_str[STR_LENGTH_OLED];
    char freq_str[STR_LENGTH_OLED];
    switch(mode){
        case MODE_AUTO:
            sprintf(mode_str, "Mode: automatic ");
            break;
        case MODE_MANU:
            sprintf(mode_str, "Mode: manual    ");
            break;
        default:
            sprintf(mode_str, "Mode: undefined ");
            break;
    }
    
    sprintf(temp_str, "Temp: %02d.%03d C  ",temp/1000,temp%1000);
    sprintf(freq_str, "Freq: %02d Hz     ",freq);

    ssd1306_set_position (0,5);
    ssd1306_puts(mode_str);
    ssd1306_set_position (0,6);
    ssd1306_puts(temp_str);
    ssd1306_set_position (0,7);
    ssd1306_puts(freq_str);
}

// Read/Write sysfs -----------------------------------------------------------

int read_temp(void){
    char buff[10];
    int temp;
    int f;
    if((f = open(TEMP_PATH, O_RDONLY)) < 0){
        syslog(LOG_INFO,"open: error read_temp\n");
        return 0;
    }
    read(f, buff, sizeof(buff) - 1);
    buff[sizeof(buff) - 1] = '\0';
    sscanf(buff, "%d", &temp);
    close(f);
    return temp;
}

int read_freq(void){
    char buff[10];
    int freq;
    int f;
    if((f = open(FREQ_PATH, O_RDONLY)) < 0){
        syslog(LOG_INFO,"open: error read_freq\n");
        return 0;
    }
    read(f, buff, sizeof(buff) - 1);
    buff[sizeof(buff) - 1] = '\0';
    sscanf(buff, "%d", &freq);
    close(f);
    return freq;
}

void write_freq(int freq){
    char buff[10];
    int f;
    if((f = open(FREQ_PATH, O_WRONLY)) < 0){
        syslog(LOG_INFO,"open: error write_freq\n");
        return;
    }
    if(freq > 99)
        freq = 99;
    sprintf(buff, "%02d", freq);
    write(f, buff, 3);
    close(f);
}

MODE read_mode(void){
    char buff[10] = {0};
    int f;
    if((f = open(MODE_PATH, O_RDONLY)) < 0){
        syslog(LOG_INFO,"open: error read_mode\n");
        return MODE_ERR;
    }
    read(f, buff, sizeof(buff) - 1);
    buff[sizeof(buff) - 1] = '\n';
    close(f);
    if(!strcmp(buff, "auto\n"))
        return MODE_AUTO;
    else if(!strcmp(buff, "man\n"))
        return MODE_MANU;
    else   
        return MODE_ERR;
}

void write_mode(MODE mode){
    int f;
    if((f = open(MODE_PATH, O_WRONLY)) < 0){
        syslog(LOG_INFO,"open: error write_mode\n");
        return;
    }
    switch(mode){
        case MODE_AUTO:
            write(f, "auto\n", 5);
            break;
        case MODE_MANU:
            write(f, "man\n", 4);
            break;
        default:
            syslog(LOG_INFO,"Error: mode unknown");
            break;
    }   
}

// Socket ---------------------------------------------------------------------

static void set_sockaddr(struct sockaddr_in *addr){
	bzero((char *)addr, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = INADDR_ANY;
	addr->sin_port = htons(DEFAULT_PORT);
}

static int setnonblocking(int sockfd){
	if (fcntl(sockfd, F_SETFD, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) ==
	    -1) {
		return -1;
	}
	return 0;
}

// Event functions ------------------------------------------------------------

void fct_K1(int epfd, event_ctrl* event){
    int freq;
    if((freq = read_freq()) < 99 && read_mode() == MODE_MANU){
        write_freq(freq+1);
        syslog(LOG_INFO,"K1, new frequency: %d\n", freq+1);
    }
    start_timer(epfd, event[TIMER_LED], TIMING_LED);
    power_led_on();
}

void fct_K2(int epfd, event_ctrl* event){
    int freq;
    if((freq = read_freq()) > 0 && read_mode() == MODE_MANU){
        write_freq(freq-1);
        syslog(LOG_INFO,"K2, new frequency: %d\n", freq-1);
    }
    start_timer(epfd, event[TIMER_LED], TIMING_LED);
    power_led_on();
}

void fct_K3(int epfd, event_ctrl* event){
    MODE mode = read_mode();
    if(mode == MODE_AUTO){
        write_mode(MODE_MANU);
        syslog(LOG_INFO,"K3, new mode: manual\n");
    }else if(mode == MODE_MANU){
        write_mode(MODE_AUTO);
        syslog(LOG_INFO,"K3, new mode: automatic\n");
    }
}

void fct_timer_oled(int epfd, event_ctrl* event){
    int temp, freq;
    MODE mode;
    set_timer(event[TIMER_OLED].fd, PERIOD_OLED);
    temp = read_temp();
    freq = read_freq();
    mode = read_mode();
    refresh_oled(mode, temp, freq);
}

void fct_timer_led(int epfd, event_ctrl* event){
    stop_timer(epfd, event[TIMER_LED]);
    power_led_off();
}

void fct_socket_conn(int epfd, event_ctrl* event){
    int n;
    char buf[BUF_SIZE];
    char parameter_str[BUF_SIZE];
    char value_str[BUF_SIZE];
    int value_freq;

    bzero(buf, sizeof(buf));
    n = read(event[SOCKET_CONN].fd, buf, sizeof(buf));
    if(n > 0){
        strcpy(parameter_str, buf);
        strtok(parameter_str, "=");
        strcpy(value_str, (char*)buf+strlen(parameter_str)+1);
        if(!strcmp(parameter_str,"frequency")){
            if(sscanf(value_str, "%d", &value_freq) == 1){
                if(read_mode() == MODE_MANU){
                    if(value_freq < 0)
                        value_freq = 0;
                    else if(value_freq > 99)
                        value_freq = 99;
                    write_freq(value_freq);
                    syslog(LOG_INFO,"Change frequency: %d\n", value_freq);
                }
            }else{
                syslog(LOG_INFO,"Frequency value not recognized: %s\n", value_str);
            }
        }else if(!strcmp(parameter_str,"mode")){
            if(!strcmp(value_str,"manual\n")){
                write_mode(MODE_MANU);
                syslog(LOG_INFO,"Change mode: manual\n");
            }else if(!strcmp(value_str,"automatic\n")){
                write_mode(MODE_AUTO);
                syslog(LOG_INFO,"Change mode: automatic\n");
            }else{
                syslog(LOG_INFO,"Mode not recognized: %s\n", value_str);
            }
        }else{
            syslog(LOG_INFO,"Unknown parameter: %s\n", parameter_str);
        }
    }

    // close connection
    syslog(LOG_INFO,"[+] connection closed\n");
    epoll_ctl(epfd, EPOLL_CTL_DEL, event[SOCKET_CONN].fd, NULL);
    close(event[SOCKET_CONN].fd);
}

void fct_socket(int epfd, event_ctrl* event){
    int socklen;
    char buf[BUF_SIZE];
    struct sockaddr_in cli_addr;

    /* handle new connection */
    event[SOCKET_CONN].fd = accept(event[SOCKET].fd, (struct sockaddr *)&cli_addr, (void*)&socklen);
    event[SOCKET_CONN].fn_ptr = &fct_socket_conn;
    event[SOCKET_CONN].event.data.ptr = &event[SOCKET_CONN];
    event[SOCKET_CONN].id = SOCKET_CONN;

    inet_ntop(AF_INET, (char *)&(cli_addr.sin_addr), buf, sizeof(cli_addr));
    syslog(LOG_INFO,"[+] connected with %s:%d\n", buf, ntohs(cli_addr.sin_port));
    setnonblocking(event[SOCKET_CONN].fd);
    
    event[SOCKET_CONN].event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, event[SOCKET_CONN].fd, &event[SOCKET_CONN].event))
        syslog(LOG_INFO,"epoll_ctl: error, event conn_sock\n");
}

// Main -----------------------------------------------------------------------

int main()
{
    event_ctrl event_list[NBR_EVENT], *event;
    struct epoll_event events[NBR_EVENT];
    struct sockaddr_in srv_addr;

    event_list[K1].fd = open_switch(SWITCH_K1);
    event_list[K2].fd = open_switch(SWITCH_K2);
    event_list[K3].fd = open_switch(SWITCH_K3);
    event_list[TIMER_OLED].fd = timerfd_create(CLOCK_MONOTONIC, 0);
    event_list[TIMER_LED].fd = timerfd_create(CLOCK_MONOTONIC, 0);

    event_list[K1].fn_ptr = &fct_K1;
    event_list[K2].fn_ptr = &fct_K2;
    event_list[K3].fn_ptr = &fct_K3;
    event_list[TIMER_OLED].fn_ptr = &fct_timer_oled;
    event_list[TIMER_LED].fn_ptr = &fct_timer_led;

    event_list[SOCKET].fd = socket(AF_INET, SOCK_STREAM, 0);
    set_sockaddr(&srv_addr);
    bind(event_list[SOCKET].fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    setnonblocking(event_list[SOCKET].fd);
    listen(event_list[SOCKET].fd, MAX_CONN);
    event_list[SOCKET].fn_ptr = &fct_socket;

    int epfd = epoll_create1(0);

    for(int id = 0; id < NBR_EVENT-1; id++){
        if(id == K1 || id == K2 || id == K3){
            event_list[id].event.events = EPOLLET;
        }else if(id == SOCKET){
            event_list[id].event.events = EPOLLIN | EPOLLOUT | EPOLLET;
        }else{
            event_list[id].event.events = 0;
        }
        event_list[id].event.data.ptr = &event_list[id];
        event_list[id].id = id;
        epoll_ctl(epfd, EPOLL_CTL_ADD, event_list[id].fd, &event_list[id].event);
    }
    int nr = epoll_wait(epfd, events, NBR_EVENT, -1);

    // Init
    openlog("thermal_control_interface",LOG_PID,LOG_INFO);
    oled_init();
    power_led_init();
    start_timer(epfd, event_list[TIMER_OLED], PERIOD_OLED);
    
    while(1){
        nr = epoll_wait(epfd, events, NBR_EVENT, -1);
        if(nr < 0){
            syslog(LOG_INFO,"epoll_wait: error\n");
        }else{
            for(int i = 0; i < nr; i++){
                event = events[i].data.ptr;   
                (*(event->fn_ptr))(epfd, event_list);
            }
        }
    }
    return 0;
}