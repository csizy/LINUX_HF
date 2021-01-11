/* 
 * FileName:    myserver.c
 * Author:      Adam Csizy
 * Neptun Code: ******
 * 
 * Desc.:       Main thread of the server application.
 * 
 * Compile like this:
 * 
 * gcc -DSERVER_DEBUG -DBME280_FLOAT_ENABLE -O0 -ggdb -Wall -o myserver myserver.c thread.c services.c bme280_qt_interf_v2.c bme280.c -pthread -I/home/lprog/MyLinuxProg/LinuxHomework/Server
 * 
 * Run like this: ./myserver (depending on the current directory you might run it as sudo)
 * 
 */

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bme280_qt_interf_v2.h"
#include "myserver.h"

/* Declare global variables */

int serverSocket;
int savedDataFd = SAVED_DATA_FD_INVALID;
char savedDataFilePath[SAVED_DATA_FILE_PATH_LEN];
pthread_mutex_t serviceMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sensorMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t savedDataMutex = PTHREAD_MUTEX_INITIALIZER;

/* Sensor and measurement settings */

struct identifier sensorId;     // Sensor interface identifier
struct bme280_dev sensorDev;    // Sensor device and settings
uint8_t sensorSettingSel;       // Sensor setting selection
uint32_t minDelay;              // Minimal delay after requesting a measurement
int measPeriod;                 // Measurement period [sec]

int main(int argc, char* argv[]) {
    
    struct sockaddr_in6 serverAddress;
    pthread_t measureThread;
    pthread_t threadPool[SERVER_THREAD_POOL_SIZE];
    
    int i;
    int *measureThreadId = NULL;
    int *serviceThreadId = NULL;
    int reuseAddrState = 1;
    
#ifndef SERVER_DEBUG
    /* Run program as system daemon */
    if(daemon(0, 0) < 0) {
        
        /* Failed to create daemon */
        perror("daemon");
        fflush(stderr);
        
        return EXIT_FAILURE;
    }
#endif
    
    /* Open connection to the system logger */
    openlog(SERVER_SYSLOG_NAME, LOG_PID | LOG_NDELAY, LOG_DAEMON);
    
    /* Log startup. */
    syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_SERVER_START);
    
    /* Initialize saved data file path */
    memset(savedDataFilePath, 0 ,sizeof(savedDataFilePath));
#ifdef SERVER_DEBUG
    
    /* Use actual directory for debugging mode */
    initSavedDataFilePath(savedDataFilePath);
#else
    
    /* Use /home/pi/ directory for daemon mode */
    strcpy(savedDataFilePath, SAVED_DATA_FILE_PATH_PI);
#endif
    
    /*
     * Note: If the current directory requires elevated rights
     * you might consider running the server as sudo
     */
    
    /* Create server socket based on the settings (TCP, IPv6) */
    if((serverSocket = socket(PF_INET6, SOCK_STREAM, 0)) < 0) {
        
        /* Failed to create server socket */
#ifdef SERVER_DEBUG
        perror("socket");
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_CREAT_FAIL);
        closelog();
        
        return EXIT_FAILURE;
    }
    
    /* Set socket option SO_REUSABLE for port reusability */
    if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddrState, sizeof(reuseAddrState)) < 0) {
        
        /* Failed to set socket option SO_REUSABLE */
#ifdef SERVER_DEBUG
        perror("setsockopt");
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_OPT_FAIL);
        
        /* Close server socket */
        if(close(serverSocket) < 0) {
        
            /* Failed to close server socket */
#ifdef SERVER_DEBUG
            perror("close");
            fflush(stderr);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_CLOSE_FAIL);
        
            /* Let the OS deal with it after terminating the program */
        }
        
        closelog();
        
        return EXIT_FAILURE;
    }
    
    /* Configure IPv6 address structure */
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin6_family = AF_INET6;
    serverAddress.sin6_addr = in6addr_any;
    serverAddress.sin6_port = htons(SERVER_PORT_NUMBER);
    
    /* Bind server socket to server address */
    if(bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        
        /* Failed to bind server socket to server address structure */
#ifdef SERVER_DEBUG
        perror("bind"); 
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_BIND_FAIL);
        
        /* Close server socket */
        if(close(serverSocket) < 0) {
        
            /* Failed to close server socket */
#ifdef SERVER_DEBUG
            perror("close");
            fflush(stderr);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_CLOSE_FAIL);
        
            /* Let the OS deal with it after terminating the program */
        }
        
        closelog();
        
        return EXIT_FAILURE;
    }
    
    /* Set server socket to passive (be able to accept incoming connections) */
    if(listen(serverSocket, SERVER_PENDING_QUEUE_LIMIT) < 0) {
        
        /* Failed to set server socket to passive */
#ifdef SERVER_DEBUG
        perror("listen");
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_LISTEN_FAIL);
        
        /* Close server socket */
        if(close(serverSocket) < 0) {
        
            /* Failed to close server socket */
#ifdef SERVER_DEBUG
            perror("close");
            fflush(stderr);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_CLOSE_FAIL);
        
            /* Let the OS deal with it after terminating the program */
        }
        
        closelog();
        
        return EXIT_FAILURE;
    }
    
    /* Initialize BME280 sensor for weather monitoring */
    if(init_dev_weather(&sensorId, &sensorDev) < 0) {
		 
        /* Failed to initialize BME280 sensor */
#ifdef SERVER_DEBUG
        fprintf(stderr, LOG_SYS_ERR_SENS_INIT_FAIL);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SENS_INIT_FAIL);
        
        closelog();
        
        return EXIT_FAILURE;
	 }
    
    /* Calculate minimal delay */
    minDelay = get_min_delay(&sensorId, &sensorDev);
    
    /* Initialize measurement period */
    measPeriod = MEAS_PERIOD_INIT_SEC;
    
    /* Initialize sensor setting selection */
    sensorSettingSel = 0;
    sensorSettingSel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;
    
    /* Create measure thread */
    measureThreadId = (int*)malloc(sizeof(int));
    *measureThreadId = 0;
    if(0 != pthread_create(&measureThread, NULL, measureThreadFunction, measureThreadId)) {
        
        /* Failed to create measure thread */
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_THREAD_MEAS_CREAT_FAIL);
        
        /* Close server socket */
        if(close(serverSocket) < 0) {
        
            /* Failed to close server socket */
#ifdef SERVER_DEBUG
            perror("close");
            fflush(stderr);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_CLOSE_FAIL);
        
            /* Let the OS deal with it after terminating the program */
        }
        
        closelog();
        
        return EXIT_FAILURE;
    }
    
    /* Create service threads */
    for(i = 0; i < SERVER_THREAD_POOL_SIZE; i++) {
        
        serviceThreadId = (int*)malloc(sizeof(int));
        *serviceThreadId = (i + 1);
        if(0 != pthread_create(&(threadPool[i]), NULL, serviceThreadFunction, serviceThreadId)) {
            
            /* Failed to create service thread */
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_THREAD_SERV_CREAT_FAIL);
        }
    }
    
    /* Main loop */
    while(1) {
        
        /* Idle on main thread */
        sleep(1);
    }
    
    /* Close server socket */
    if(close(serverSocket) < 0) {
        
        /* Failed to close server socket */
#ifdef SERVER_DEBUG
        perror("close");
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_CLOSE_FAIL);
        
        /* Let the OS deal with it after terminating the program */
    }
    
    // TODO Kill service and measure threads 
    
    /* Close saved data file */
    close(savedDataFd);
    savedDataFd = SAVED_DATA_FD_INVALID;
    
    /* Close connection to the system logger */
    closelog();
    
    return EXIT_SUCCESS;
}
