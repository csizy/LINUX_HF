/* 
 * FileName:    thread.c
 * Author:      Adam Csizy
 * Neptun Code: ******
 * 
 * Desc.:       Source file containing function definitions implementing the
 *              measurement and client service threads.
 */

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bme280_qt_interf_v2.h"
#include "myserver.h"

/* Function definitions */

/*
 * Function 'serviceThreadFunction': serves client connections.
 */
void* serviceThreadFunction(void *arg) {
    
    int errorCode;
    int keepAliveState = 1;
    int len;
    int serviceStopCondition = COND_SERVICE_STOP_FALSE;
    int serviceSocket;
    int threadId;
    
    uint8_t response;
    
    char username[USR_NAME_MAX_LENGTH];
    char password[USR_PWD_MAX_LENGTH];
    
    char clientHostName[NI_MAXHOST];
    char clientServiceName[NI_MAXSERV];
    
    const struct UserData *userRef = NULL;
    
    struct pollfd pollArray[POLL_ARRAY_SIZE];
    
    struct sockaddr_in6 clientAddress;
    socklen_t clientAddressLength;
    
    /* Set thread id for log purposes */
    threadId = *((int*)arg);
    free(arg);
    
    /* Initialize local variables */
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));
    memset(clientHostName, 0, sizeof(clientHostName));
    memset(clientServiceName, 0, sizeof(clientServiceName));
    
    while(1) {
        
        /* Try to lock service mutex and wait for client connection */
        pthread_mutex_lock(&serviceMutex);
        serviceSocket = accept(serverSocket, (struct sockaddr*)(&clientAddress), &clientAddressLength);
        pthread_mutex_unlock(&serviceMutex);
        
        /* Check success of accepting new connection */
        if(serviceSocket < 0) {
            
            /* Failed to accept incoming connection */
#ifdef SERVER_DEBUG
            perror("accept");
            fflush(stderr);
            fprintf(stdout, LOG_SYS_ERR_SERVER_SOCK_ACCEPT_FAIL, threadId);
            fflush(stdout);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SOCK_ACCEPT_FAIL, threadId);
        }
        else {
            
            /* Succeeded to accept client connection */
            
            /* Try to resolve client name by address */
            if(0 == (errorCode = getnameinfo(
                
                (struct sockaddr*)(&clientAddress),
                clientAddressLength,
                clientHostName,
                sizeof(clientHostName),
                clientServiceName,
                sizeof(clientServiceName),
                NI_NAMEREQD | NI_NUMERICHOST | NI_NUMERICSERV
                )
                
            )) {
                
                /* Resolved client host name */
                
#ifdef SERVER_DEBUG
                fprintf(stdout, LOG_SYS_INFO_CLIENT_CONN, threadId, clientHostName, clientServiceName);
                fflush(stdout);
#endif
                syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_CONN, threadId, clientHostName, clientServiceName);
            }
            else {
                
                /* Failed to resolve client host name */
#ifdef SERVER_DEBUG
                fprintf(stderr, LOG_SYS_WARN_CLIENT_NAME_RESOLVE_FAIL, threadId);
                fprintf(stderr, "%s \n", gai_strerror(errorCode));
                fflush(stderr);
#endif
                syslog(LOG_DAEMON | LOG_WARNING, LOG_SYS_WARN_CLIENT_NAME_RESOLVE_FAIL, threadId);
            }
            
            /* Set service socket option SO_KEEPALIVE for enhanced safety */
            if(setsockopt(serviceSocket, SOL_SOCKET, SO_KEEPALIVE, &keepAliveState, sizeof(keepAliveState)) < 0) {
                
                /* Failed to set socket option SO_KEEPALIVE */
#ifdef SERVER_DEBUG
                perror("setsockopt");
                fflush(stderr);
                fprintf(stderr, LOG_SYS_WARN_SOCK_SET_OPT_FAIL, threadId);
                fflush(stderr);
#endif
                syslog(LOG_DAEMON | LOG_WARNING, LOG_SYS_WARN_SOCK_SET_OPT_FAIL, threadId);
            }
            
            /* Authenticate client */
            len = recv(serviceSocket, username, sizeof(username), MSG_WAITALL);
            len = recv(serviceSocket, password, sizeof(password), MSG_WAITALL);
            
            if(0 != authClient(username, password, &userRef)) {
                
                /* Client authentication failed */
#ifdef SERVER_DEBUG
                fprintf(stdout, LOG_SYS_INFO_CLIENT_AUTH_FAIL, threadId);
                fflush(stdout);
#endif
                syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_AUTH_FAIL, threadId);
                response = RES_CODE_AUTH_FAIL;
                len = send(serviceSocket, &response, sizeof(response), MSG_NOSIGNAL);
                if(len < 0) {
                 
                    /* Failed to notify client of unsuccessful authentication */
#ifdef SERVER_DEBUG
                    perror("send");
                    fflush(stderr);
                    fprintf(stderr, LOG_SYS_INFO_CLIENT_AUTH_FAIL_NOTIF_FAIL, threadId);
                    fflush(stderr);
#endif
                    syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_INFO_CLIENT_AUTH_FAIL_NOTIF_FAIL, threadId);
                }
            }
            else {
                
                /* Client authentication succeded */
#ifdef SERVER_DEBUG
                fprintf(stdout, LOG_SYS_INFO_CLIENT_AUTH_SUCCESS, threadId, userRef->name);
                fflush(stdout);
#endif
                syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_AUTH_SUCCESS, threadId, userRef->name);
                response = RES_CODE_AUTH_SUCCESS;
                len = send(serviceSocket, &response, sizeof(response), MSG_NOSIGNAL);
                if(len < 0) {
                 
                    /* Failed to notify client of successful authentication */
#ifdef SERVER_DEBUG
                    perror("send");
                    fflush(stderr);
                    fprintf(stderr, LOG_SYS_INFO_CLIENT_AUTH_SUCCESS_NOTIF_FAIL, threadId);
                    fflush(stderr);
#endif
                    syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_INFO_CLIENT_AUTH_SUCCESS_NOTIF_FAIL, threadId);
                }
                
                /* Initialize poll client */
                pollArray[POLL_ARRAY_SOCKET].fd = serviceSocket;
                pollArray[POLL_ARRAY_SOCKET].events = POLLIN;
            
                serviceStopCondition = COND_SERVICE_STOP_FALSE;
                
                while(!serviceStopCondition) {
                
                    /* Polling... */
                    if(poll(pollArray, POLL_ARRAY_SIZE, -1) > 0) {
                    
                        if(pollArray[POLL_ARRAY_SOCKET].revents & (POLLERR | POLLHUP)) {
                        
                            /* Client closed connection */
#ifdef SERVER_DEBUG
                            fprintf(stdout, LOG_SYS_WARN_CLIENT_DISCONN_UNEX, threadId);
                            fflush(stdout);
#endif
                            syslog(LOG_DAEMON | LOG_WARNING, LOG_SYS_WARN_CLIENT_DISCONN_UNEX, threadId);
                            serviceStopCondition = COND_SERVICE_STOP_TRUE;
                        }
                        else if (pollArray[POLL_ARRAY_SOCKET].revents & (POLLIN)) {
                        
                            /* Message received from client */
                            clientHandler(pollArray[POLL_ARRAY_SOCKET].fd, userRef, &serviceStopCondition);
                        }
                    }
                }
            }
            
            /* Close service socket */
            close(serviceSocket);
            pollArray[POLL_ARRAY_SOCKET].fd = -1;
            
            /* Syslog end of current service */
#ifdef SERVER_DEBUG
            fprintf(stdout, LOG_SYS_INFO_THREAD_SERVICE_END, threadId);
            fflush(stdout);
#endif
            syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_THREAD_SERVICE_END, threadId);
        }
    }
    
    return EXIT_SUCCESS;
}

/*
 * Function 'measureThreadFunction': conducts consecutive measurements.
 */
void* measureThreadFunction(void *arg) {
    
    const float invalidValue = -1.0;            // Invalid measurement value 
    
    uint8_t copy_of_sensorSettingSel = 0;       // Copy of sensor setting selection
    int copy_of_measPeriod = 0;                 // Copy of measurement period [sec]
    int error = 0;
    int measFlag = 1;                           // Measurement flag
    int threadId;
    struct sensor_data measData;                // Measured sensor data
    
    /* Set thread id for log purposes */
    threadId = *((int*)arg);
    free(arg);
    
    /* Loop */
    while(1) {
        
        if(measFlag) {
        
            /* Start of critical section */
            pthread_mutex_lock(&sensorMutex);
        
            /* Update minimal delay */
            minDelay = get_min_delay(&sensorId, &sensorDev);
        
            /* Conduct measurement */
            error = get_sensor_data(&sensorId, &sensorDev, minDelay, &measData);
        
            /* Copy sensor setting selection */
            copy_of_sensorSettingSel = sensorSettingSel;
        
            /* Copy measurement period */
            copy_of_measPeriod = measPeriod;
        
            /* End of critical section */
            pthread_mutex_unlock(&sensorMutex);
        
            /* Flush potential sensor interface logs */
#ifdef SERVER_DEBUG
            fflush(stderr);
            fflush(stdout);
#endif
            if(0 != error) {
                
                /* Failed to get BME280 sensor data */
#ifdef SERVER_DEBUG
                fprintf(stderr, LOG_SYS_ERR_SENS_MEAS_FAIL);
                fflush(stderr);
#endif
                syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SENS_MEAS_FAIL);
            }
            else {
                
                /* Got BME280 sensor data */
#ifdef SERVER_DEBUG
                //fprintf(stdout, LOG_SYS_INFO_SENS_MEAS_SUCCESS);
                //fflush(stdout);
#endif
                syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_SENS_MEAS_SUCCESS);
        
                /* Save measurement data */
                
                pthread_mutex_lock(&savedDataMutex);
                
                /* Open saved data file if not yet open */
                if(savedDataFd < 0) {
                 
                    /* File needs to be opened */
                    savedDataFd = open(savedDataFilePath, O_CREAT | O_TRUNC | O_RDWR, 0644);
                    if(savedDataFd < 0) {
                    
#ifdef SERVER_DEBUG
                        perror("open");
                        fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_OPEN_FAIL);
                        fflush(stderr);
#endif
                        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_OPEN_FAIL);
                    }
                }
                
                /* Set file offset to end of file */
                lseek(savedDataFd, 0, SEEK_END);
                
                /* Save temperature data */
                if(BME280_OSR_TEMP_SEL & copy_of_sensorSettingSel) {
                    
                    /* Temperature measurement selected */
                    if(sizeof(measData.temp) != write(savedDataFd, &(measData.temp), sizeof(measData.temp))) {
                        
#ifdef SERVER_DEBUG
                        perror("write");
                        fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_TEMP_FAIL);
                        fflush(stderr);
#endif
                        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_TEMP_FAIL);
                    }
                }
                else {
                    
                    /* Temperature measurement not selected */
                    if(sizeof(invalidValue) != write(savedDataFd, &invalidValue, sizeof(invalidValue))) {
                        
#ifdef SERVER_DEBUG
                        perror("write");
                        fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_TEMP_FAIL);
                        fflush(stderr);
#endif
                        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_TEMP_FAIL);
                    }
                }
                
                /* Save humidity data */
                if(BME280_OSR_HUM_SEL & copy_of_sensorSettingSel) {
                    
                    /* Humidity measurement selected */
                    if(sizeof(measData.hum) != write(savedDataFd, &(measData.hum), sizeof(measData.hum))) {
                        
#ifdef SERVER_DEBUG
                        perror("write");
                        fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_HUMID_FAIL);
                        fflush(stderr);
#endif
                        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_HUMID_FAIL);
                    }
                }
                else {
                    
                    /* Humidity measurement not selected */
                    if(sizeof(invalidValue) != write(savedDataFd, &invalidValue, sizeof(invalidValue))) {
                        
#ifdef SERVER_DEBUG
                        perror("write");
                        fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_HUMID_FAIL);
                        fflush(stderr);
#endif
                        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_HUMID_FAIL);
                    }
                }
                
                /* Save pressure data */
                if(BME280_OSR_PRESS_SEL & copy_of_sensorSettingSel) {
                    
                    /* Pressure measurement selected */
                    if(sizeof(measData.press) != write(savedDataFd, &(measData.press), sizeof(measData.press))) {
                        
#ifdef SERVER_DEBUG
                        perror("write");
                        fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_PRESS_FAIL);
                        fflush(stderr);
#endif
                        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_PRESS_FAIL);
                    }
                }
                else {
                    
                    /* Pressure measurement not selected */
                    if(sizeof(invalidValue) != write(savedDataFd, &invalidValue, sizeof(invalidValue))) {
                        
#ifdef SERVER_DEBUG
                        perror("write");
                        fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_PRESS_FAIL);
                        fflush(stderr);
#endif
                        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_PRESS_FAIL);
                    }
                }
                
                pthread_mutex_unlock(&savedDataMutex);
                
                // TODO Fix bug: if period is changed from 1h to 1sec it will only be updated after the 1 hour elapsed
                // TODO Fix bug: something like a condition change listener or signalling.
                // Period updated -> Measurement thread notified -> Measurement conducted right away -> sleep for new period
            }
        
        }
        
        /* Check value of period */
        if(0 == copy_of_measPeriod) {
            
            /* Default 2 sec */
            sleep(2);
            measFlag = 0;
        }
        else {
            
            /* Sleep for the given period */
            sleep(copy_of_measPeriod);
            measFlag = 1;
        }
    }
    
    return EXIT_SUCCESS;
}
