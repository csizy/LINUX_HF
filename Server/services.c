/* 
 * FileName:    services.c
 * Author:      Adam Csizy
 * Neptun Code: ******
 * 
 * Desc.:       Source file containing function definitions supporting various
 *              purposes including handling client requests, authentication
 *              and other utilities.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bme280_qt_interf_v2.h"
#include "myserver.h"

/* Global variable definitions */

/* Const array containing user data */
const struct UserData userDataArray[USR_DATA_ARRAY_SIZE] = {
 
    {"user1", "pass1", USR_GRP_CONF},
    {"user2", "pass2", USR_GRP_GUEST},
    {"user3", "pass3", USR_GRP_CONF}
};

/* Const array containing request code - groupe permission pairs */
const struct Request requestArray[REQ_ARRAY_SIZE] = {
    
    {REQ_CODE_DCONN, disconnectHandler, USR_GRP_GUEST},
    {REQ_CODE_SCONF, setConfigHandler, USR_GRP_CONF},
    {REQ_CODE_GCONF, getConfigHandler, USR_GRP_GUEST},
    {REQ_CODE_RMDAT, removeDataHandler, USR_GRP_CONF},
    {REQ_CODE_GDAT, getDataHandler, USR_GRP_GUEST}
};

/* Function definitions */

/*
 * Function 'authClient': authenticates client before connetion.
 */
int authClient(const char *username, const char *password, const struct UserData* *userRef) {
 
    int error = 0;
    int i;
    
    for(i = 0; i < USR_DATA_ARRAY_SIZE; i++) {
        
        if(0 == strcmp(userDataArray[i].name, username)) {
            
            /* Username match */
            
            if(0 == strcmp(userDataArray[i].password, password)) {
                
                /* Authentication succeeded */
                *userRef = &(userDataArray[i]);
                return error;
            }
            else {
                
                /* Authentication failed */
                *userRef = NULL;
                error = -1;
                return error;
            }
        }
    }
    
    *userRef = NULL;
    error = -1;
    return error;
}

/*
 * Function 'initSavedDataFilePath': initializes file path for file containing saved data.
 */
int initSavedDataFilePath(char filePathBuf[]) {
    
    int error = 0;
    char *ret = NULL;
    
    /* Get current working directory */
    ret = getcwd(filePathBuf, SAVED_DATA_FILE_PATH_LEN);
    if(NULL == ret) {
        
        /* Failed to get current working directory */
#ifdef SERVER_DEBUG
        perror("getcwd");
        fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_PATH_INIT_FAIL);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_PATH_INIT_FAIL);
        
        error = -1;
        return error;
    }
    
    /* Concatenate file name to directory path */
    strcat(filePathBuf, "/");
    strcat(filePathBuf, SAVED_DATA_FILE_NAME);
    
    return error;
}

/*
 * Function 'clientHandler': interprets and forwards client requests.
 * 
 * Note:    The clientHandler will notify client in case of: 
 *          granted permission, denied permission, invalid permission and
 *          failure in request handling.
 *          
 *          The handler functions should not notify clients in case of
 *          failure in request handling (server side error). Instead signal
 *          the errors with their return value and let the main clientHandler
 *          deal with it. Handler functions may notify clients of successful
 *          request handlings (protocol defined).
 */
int clientHandler(int serviceSocket, const struct UserData *user, int *serviceStopCondition) {
    
    uint8_t requestCode;
    uint8_t response;
    int iReq;
    int len;
    int error = 0;
    
    /* Check user and stop condition argument */
    if((NULL == user) || (NULL == serviceStopCondition)) {
        
        /* Invalid user or stop service argument */
#ifdef SERVER_DEBUG
        fprintf(stdout, LOG_SYS_ERR_INVALID_ARGS);
        fflush(stdout);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_INVALID_ARGS);
        error = -1;
        return error;
    }
    
    /* Check client socket argument */
    if(serviceSocket < 0) {
        
        /* Invalid client socket argument */
#ifdef SERVER_DEBUG
        fprintf(stdout, LOG_SYS_ERR_INVALID_SOCK);
        fflush(stdout);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_INVALID_SOCK);
        error = -1;
        return error;
    }
    
    /* Receive client request */
    len = recv(serviceSocket, &requestCode, sizeof(requestCode), MSG_WAITALL);
    if(len < 0) {
        
        /* Failed to receive client request */
#ifdef SERVER_DEBUG
        perror("recv");
        fflush(stderr);
        fprintf(stdout, LOG_SYS_ERR_CLIENT_REQ_RECV_FAIL);
        fflush(stdout);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_CLIENT_REQ_RECV_FAIL);
        
        error = -1;
        return error;
    }
  
    /* Identify client request */
    for(iReq = 0; iReq < REQ_ARRAY_SIZE; iReq++) {
         
        if(requestArray[iReq].requestCode == requestCode) {
                
            /* Client request code identified */
                
            /* Check user permission */
            if(requestArray[iReq].lowestGroupe <= user->groupe) {
                    
                /* Permission granted */
                    
                /* Notify client (permission granted) */
                response = RES_CODE_REQ_ACCEPT;
                len = send(serviceSocket, &response, sizeof(response), MSG_NOSIGNAL);
                if(len < 0) {
                        
                    /* Failed to send server response to client */
#ifdef SERVER_DEBUG
                    perror("send");
                    fflush(stderr);
                    fprintf(stdout, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
                    fflush(stdout);
#endif
                    syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
                    error = -1;
                }
                    
                /* Invoke request handler */
                error = requestArray[iReq].requestHandler(serviceSocket, user, serviceStopCondition);
                if(error != 0) {
                        
                    /* Failed to accomplish client request */
#ifdef SERVER_DEBUG
                    fprintf(stdout, LOG_SYS_ERR_CLIENT_REQ_FAIL, user->name, requestCode);
                    fflush(stdout);
#endif
                    syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_CLIENT_REQ_FAIL, user->name, requestCode);
                        
                    /* Notify client (failed to accomplish request) */
                    response = RES_CODE_REQ_FAIL;
                    len = send(serviceSocket, &response, sizeof(response), MSG_NOSIGNAL);
                    if(len < 0) {
                        
                        /* Failed to send server response to client */
#ifdef SERVER_DEBUG
                        perror("send");
                        fflush(stderr);
                        fprintf(stdout, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
                        fflush(stdout);
#endif
                        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
                        error = -1;
                        return error;
                    }
                }
                    
                // Q: Shall we notify the client here about the success ? (RES_CODE_REQ_SUCCESS)
                // A: No. Success shall be notified inside handler.
            }
            else {
                    
                /* Permission denied */
#ifdef SERVER_DEBUG
                fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_NO_PERM, user->name, requestCode);
                fflush(stdout);
#endif
                syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_NO_PERM, user->name, requestCode);
                    
                /* Notify client (no permission) */
                response = RES_CODE_REQ_NO_PERM;
                len = send(serviceSocket, &response, sizeof(response), MSG_NOSIGNAL);
                if(len < 0) {
                        
                    /* Failed to send server response to client */
#ifdef SERVER_DEBUG
                    perror("send");
                    fflush(stderr);
                    fprintf(stdout, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
                    fflush(stdout);
#endif
                    syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
                    error = -1;
                }
            }
                
            /* No need to keep looking for other request codes */
            return error;
        }
    }
    
    /* Could not identify client request code */
    
    /* Notify client (invalid request code) */
    response = RES_CODE_REQ_INVALID;
    len = send(serviceSocket, &response, sizeof(response), MSG_NOSIGNAL);
    if(len < 0) {
                        
        /* Failed to send server response to client */
#ifdef SERVER_DEBUG
        perror("send");
        fflush(stderr);
        fprintf(stdout, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
        fflush(stdout);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
        error = -1;
    }
    
    return error;
}

/*
 * Function 'disconnectHandler': handles disconnection requested by client.
 * 
 * Protocol:    Client --> Server: disconnect request code <1 byte>
 */
int disconnectHandler(int clientSocket, const struct UserData *user, void *customArg) {
    
    int error = 0;
    int *serviceStopCondition = (int*)customArg;
    
    *serviceStopCondition = COND_SERVICE_STOP_TRUE;
    
    /* Let the main service thread (serviceThreadFunction) close the socket */
    
    /* Syslog client requested disconnection */
#ifdef SERVER_DEBUG
    fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_DISCONN, user->name);
    fflush(stdout);
#endif
    syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_DISCONN, user->name);
    
    return error;
}

/*
 * Function 'setConfigHandler': sets new sensor configuration requested by client.
 * 
 * Note:        Notify the client in case of successful request handling.
 * 
 * Protocol:    Client --> Server: set configuration request code <1 byte>
 *              Client <-- Server: result of request code validation <1 byte>
 * 
 *              ++ In case of successful request validation ++
 * 
 *              Client --> Server: configuration parameters <1 byte>
 *              Client --> Server: period value <4 bytes> (only for period config)
 *              Client <-- Server: result of request processing <1 byte>
 */
int setConfigHandler(int clientSocket, const struct UserData *user, void *customArg) {
    
    uint8_t config = 0;             // Sensor configuration
    uint8_t response = 0;           // Server response
    int error = 0;
    int len;
    int period = 0;                 // Sampling period
    
    /* Syslog client requested to set sensor configuration */
#ifdef SERVER_DEBUG
    fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_SET_CONF, user->name);
    fflush(stdout);
#endif
    syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_SET_CONF, user->name);
    
    /* Get sensor config from client */
    len = recv(clientSocket, &config, sizeof(config), MSG_WAITALL);
    if(len < 0) {
        
        /* Failed to receive client config request */
#ifdef SERVER_DEBUG
        perror("recv");
        fflush(stderr);
        fprintf(stdout, LOG_SYS_ERR_CLIENT_REQ_CONF_FAIL, user->name);
        fflush(stdout);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_CLIENT_REQ_CONF_FAIL, user->name);
        
        error = -1;
        return error;
    }
    
    /* Receive period value from client based on config */
    if(REQ_CONF_PRD == (config & REQ_CONF_TYPE_MASK)) {
        
        /* Get sensor config from client */
        len = recv(clientSocket, &period, sizeof(period), MSG_WAITALL);
        if(len < 0) {
        
            /* Failed to receive period value from client */
#ifdef SERVER_DEBUG
            perror("recv");
            fflush(stderr);
            fprintf(stdout, LOG_SYS_ERR_CLIENT_REQ_RECV_FAIL);
            fflush(stdout);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_CLIENT_REQ_RECV_FAIL);
        
            error = -1;
            return error;
        }
    }
    
    /* Process client config data */
    if(REQ_CONF_PRD == (config & REQ_CONF_TYPE_MASK)) {
        
        /* Config period */

        pthread_mutex_lock(&sensorMutex);
        measPeriod = period;
        pthread_mutex_unlock(&sensorMutex);
    }
    else if(REQ_CONF_IIR == (config & REQ_CONF_TYPE_MASK)) {
        
        /* Config IIR filter */
        
        /* Process config status */
        if(REQ_CONF_ON == (config & REQ_CONF_STATUS_MASK)) {
            
            /* Config status on */
            
            /* Turn on IIR filter */
            pthread_mutex_lock(&sensorMutex);
            sensorSettingSel |= BME280_FILTER_SEL;
            pthread_mutex_unlock(&sensorMutex);
            
            /* Process config value */
            if(REQ_CONF_COEFF_OFF == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config IIR filter COEFF_OFF */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.filter = BME280_FILTER_COEFF_OFF;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_COEFF_2 == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config IIR filter COEFF_2 */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.filter = BME280_FILTER_COEFF_2;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_COEFF_4 == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config IIR filter COEFF_4 */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.filter = BME280_FILTER_COEFF_4;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_COEFF_8 == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config IIR filter COEFF_8 */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.filter = BME280_FILTER_COEFF_8;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_COEFF_16 == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config IIR filter COEFF_16 */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.filter = BME280_FILTER_COEFF_16;
                pthread_mutex_unlock(&sensorMutex);
            }
            else {
                
                /* Invalid config value */
#ifdef SERVER_DEBUG
                fprintf(stdout, LOG_SYS_ERR_CLIENT_REQ_CONF_VAL_INVAL, user->name);
                fflush(stdout);
#endif
                syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_CLIENT_REQ_CONF_VAL_INVAL, user->name);
                
                error = -1;
                return error;
            }
        }
        else {
            
            /* Config status off */
            
            /* Turn off IIR filter */
            pthread_mutex_lock(&sensorMutex);
            sensorSettingSel &= ~(BME280_FILTER_SEL);
            pthread_mutex_unlock(&sensorMutex);
        }
    }
    else if(REQ_CONF_TMP == (config & REQ_CONF_TYPE_MASK)) {
        
        /* Config temperature */
        
        /* Process config status */
        if(REQ_CONF_ON == (config & REQ_CONF_STATUS_MASK)) {
            
            /* Config status on */
            
            /* Turn on temperature measurement */
            pthread_mutex_lock(&sensorMutex);
            sensorSettingSel |= BME280_OSR_TEMP_SEL;
            pthread_mutex_unlock(&sensorMutex);
            
            /* Process config value */
            if(REQ_CONF_OS_OFF == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config temperature OS_OFF */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_t = BME280_NO_OVERSAMPLING;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_1X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config temperature OS_1X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_t = BME280_OVERSAMPLING_1X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_2X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config temperature OS_2X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_t = BME280_OVERSAMPLING_2X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_4X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config temperature OS_4X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_t = BME280_OVERSAMPLING_4X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_8X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config temperature OS_8X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_t = BME280_OVERSAMPLING_8X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_16X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config temperature OS_16X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_t = BME280_OVERSAMPLING_16X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else {
                
                /* Invalid config value */
#ifdef SERVER_DEBUG
                fprintf(stdout, LOG_SYS_ERR_CLIENT_REQ_CONF_VAL_INVAL, user->name);
                fflush(stdout);
#endif
                syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_CLIENT_REQ_CONF_VAL_INVAL, user->name);
                
                error = -1;
                return error;
            }
        }
        else {
            
            /* Config status off */
            
            /* Turn on temperature measurement */
            pthread_mutex_lock(&sensorMutex);
            sensorSettingSel &= ~(BME280_OSR_TEMP_SEL);
            pthread_mutex_unlock(&sensorMutex);
        }
    }
    else if(REQ_CONF_HUM == (config & REQ_CONF_TYPE_MASK)) {
        
        /* Config humidity */
        
        /* Process config status */
        if(REQ_CONF_ON == (config & REQ_CONF_STATUS_MASK)) {
            
            /* Config status on */
            
            /* Turn on humidity measurement */
            pthread_mutex_lock(&sensorMutex);
            sensorSettingSel |= BME280_OSR_HUM_SEL;
            pthread_mutex_unlock(&sensorMutex);
            
            /* Process config value */
            if(REQ_CONF_OS_OFF == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config humidity OS_OFF */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_h = BME280_NO_OVERSAMPLING;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_1X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config humidity OS_1X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_h = BME280_OVERSAMPLING_1X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_2X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config humidity OS_2X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_h = BME280_OVERSAMPLING_2X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_4X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config humidity OS_4X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_h = BME280_OVERSAMPLING_4X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_8X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config humidity OS_8X using mutex */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_h = BME280_OVERSAMPLING_8X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_16X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config humidity OS_16X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_h = BME280_OVERSAMPLING_16X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else {
                
                /* Invalid config value */
#ifdef SERVER_DEBUG
                fprintf(stdout, LOG_SYS_ERR_CLIENT_REQ_CONF_VAL_INVAL, user->name);
                fflush(stdout);
#endif
                syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_CLIENT_REQ_CONF_VAL_INVAL, user->name);
                
                error = -1;
                return error;
            }
        }
        else {
            
            /* Config status off */
            
            /* Turn off humidity measurement */
            pthread_mutex_lock(&sensorMutex);
            sensorSettingSel &= ~(BME280_OSR_HUM_SEL);
            pthread_mutex_unlock(&sensorMutex);
        }
    }
    else if(REQ_CONF_PRS == (config & REQ_CONF_TYPE_MASK)) {
        
        /* Config pressure */
        
        /* Process config status */
        if(REQ_CONF_ON == (config & REQ_CONF_STATUS_MASK)) {
            
            /* Config status on */
            
            /* Turn on pressure measurement */
            pthread_mutex_lock(&sensorMutex);
            sensorSettingSel |= BME280_OSR_PRESS_SEL;
            pthread_mutex_unlock(&sensorMutex);
            
            /* Process config value */
            if(REQ_CONF_OS_OFF == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config pressure OS_OFF */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_p = BME280_NO_OVERSAMPLING;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_1X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config pressure OS_1X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_p = BME280_OVERSAMPLING_1X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_2X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config pressure OS_2X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_p = BME280_OVERSAMPLING_2X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_4X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config pressure OS_4X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_p = BME280_OVERSAMPLING_4X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_8X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config pressure OS_8X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_p = BME280_OVERSAMPLING_8X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else if(REQ_CONF_OS_16X == (config & REQ_CONF_VALUE_MASK)) {
                
                /* Config pressure OS_16X */
                pthread_mutex_lock(&sensorMutex);
                sensorDev.settings.osr_p = BME280_OVERSAMPLING_16X;
                pthread_mutex_unlock(&sensorMutex);
            }
            else {
                
                /* Invalid config value */
#ifdef SERVER_DEBUG
                fprintf(stdout, LOG_SYS_ERR_CLIENT_REQ_CONF_VAL_INVAL, user->name);
                fflush(stdout);
#endif
                syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_CLIENT_REQ_CONF_VAL_INVAL, user->name);
                
                error = -1;
                return error;
            }
        }
        else {
            
            /* Config status off */
            
            /* Turn off pressure measurement */
            pthread_mutex_lock(&sensorMutex);
            sensorSettingSel &= ~(BME280_OSR_PRESS_SEL);
            pthread_mutex_unlock(&sensorMutex);
        }
    }
    else {
        
        /* Invalid config type */
#ifdef SERVER_DEBUG
        fprintf(stdout, LOG_SYS_ERR_CLIENT_REQ_CONF_TYP_INVAL, user->name);
        fflush(stdout);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_CLIENT_REQ_CONF_TYP_INVAL, user->name);
                
        error = -1;
        return error;
    }
    
    /* Update sensor settings */
    pthread_mutex_lock(&sensorMutex);
    error = bme280_set_sensor_settings(sensorSettingSel, &sensorDev); // Prevent concurrent sensor and config access
    pthread_mutex_unlock(&sensorMutex);
    if (BME280_OK != error) {
        
        /* Failed to update sensor settings */
#ifdef SERVER_DEBUG
        fprintf(stderr, "Failed to set sensor settings (code %+d).", error);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SENS_STGS_SET_FAIL);
        
        error = -1;
        return error;
    }
    
    if(0 == error) {
     
        /* Notify client of success */
        response = RES_CODE_REQ_SUCCESS;
        len = send(clientSocket, &response, sizeof(response), MSG_NOSIGNAL);
        if(len < 0) {
                        
            /* Failed to send server response to client */
#ifdef SERVER_DEBUG
            perror("send");
            fflush(stderr);
            fprintf(stdout, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
            fflush(stdout);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
            error = -1;
            return error;
        }
        
    }
    
    /* Syslog successful configuration */
#ifdef SERVER_DEBUG
    fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_SET_CONF_SUCCESS, user->name);
    fflush(stdout);
#endif
    syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_SET_CONF_SUCCESS, user->name);
    
    return error;
}

/*
 * Function 'getConfigHandler': returns current sensor configuration requested by client.
 * 
 * Protocol:    Client --> Server: get configuration request code <1 byte>
 *              Client <-- Server: result of request code validation <1 byte>
 * 
 *              ++ In case of successful request validation ++
 * 
 *              Client <-- Server: period value <4 bytes>
 *              Client <-- Server: temperature configuration <8 bytes>
 *              Client <-- Server: humidity configuration <8 bytes>
 *              Client <-- Server: pressure configuration <8 bytes>
 *              Client <-- Server: IIR filter configuration <16 bytes>
 */
int getConfigHandler(int clientSocket, const struct UserData *user, void *customArg) {
    
    uint8_t response = 0;                       // Not used
    int error = 0;
    int len;
    
    char envVarConfMsg[8];                      // String message of TMP/HUM/PRS configs (collective)
    char iirFltrConfMsg[16];                    // String message of IIR filter config
    
    struct bme280_dev copy_of_sensorDev;        // Copy of sensor device and settings
    uint8_t copy_of_sensorSettingSel = 0;       // Copy of sensor setting selection
    int copy_of_measPeriod = 0;                 // Copy of measurement period [sec]
    
    /* Initialize local variables */
    memset(envVarConfMsg, 0, sizeof(envVarConfMsg));
    memset(iirFltrConfMsg, 0, sizeof(iirFltrConfMsg));
    memset(&copy_of_sensorDev, 0, sizeof(copy_of_sensorDev));
    
    /* Syslog client requested to get sensor configuration */
#ifdef SERVER_DEBUG
    fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_GET_CONF, user->name);
    fflush(stdout);
#endif
    syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_GET_CONF, user->name);
    
    /* Copy config settings values */
    
    /* Start of critical section */
    pthread_mutex_lock(&sensorMutex);
    
    /* Copy sensor device settings */
    copy_of_sensorDev = sensorDev;
    
    /* Copy sensor setting selection */
    copy_of_sensorSettingSel = sensorSettingSel;
    
    /* Copy measurement period */
    copy_of_measPeriod = measPeriod;
    
    /* End of critical section */
    pthread_mutex_unlock(&sensorMutex);
    
    /* Send measurement period config */
    len = send(clientSocket, &copy_of_measPeriod, sizeof(copy_of_measPeriod), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send measurement period config to client */
#ifdef SERVER_DEBUG
        perror("send");
        fprintf(stderr, LOG_SYS_ERR_SERVER_CONF_PRD_SEND_FAIL, user->name);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_CONF_PRD_SEND_FAIL, user->name);
        
        error = -1;
        return error;
    }
    
    /* Parse and send temperature config */
    
    /* Parse temperature config */
    if(copy_of_sensorSettingSel & BME280_OSR_TEMP_SEL) {
        
        /* Temperature config status on */
        
        if(BME280_NO_OVERSAMPLING == copy_of_sensorDev.settings.osr_t) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_OFF);
        }
        else if(BME280_OVERSAMPLING_1X == copy_of_sensorDev.settings.osr_t) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_1X);
        }
        else if(BME280_OVERSAMPLING_2X == copy_of_sensorDev.settings.osr_t) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_2X);
        }
        else if(BME280_OVERSAMPLING_4X == copy_of_sensorDev.settings.osr_t) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_4X);
        }
        else if(BME280_OVERSAMPLING_8X == copy_of_sensorDev.settings.osr_t) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_8X);
        }
        else if(BME280_OVERSAMPLING_16X == copy_of_sensorDev.settings.osr_t) {
        
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_16X);
        }
        else {
         
            strcpy(envVarConfMsg, RES_STR_SCONF_ERR);
        }
    }
    else {
        
        /* Temperature config status off */
        strcpy(envVarConfMsg, RES_STR_SCONF_OFF);
    }
    
    /* Send temperature config */
    len = send(clientSocket, &envVarConfMsg, sizeof(envVarConfMsg), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send temperature config to client */
#ifdef SERVER_DEBUG
        perror("send");
        fprintf(stderr, LOG_SYS_ERR_SERVER_CONF_TMP_SEND_FAIL, user->name);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_CONF_TMP_SEND_FAIL, user->name);
        
        error = -1;
        return error;
    }
    
    /* Parse and send humidity config */
    
    memset(envVarConfMsg, 0, sizeof(envVarConfMsg)); // Reset shared variable
    
    /* Parse humidity config */
    if(copy_of_sensorSettingSel & BME280_OSR_HUM_SEL) {
        
        /* Humidity config status on */
        
        if(BME280_NO_OVERSAMPLING == copy_of_sensorDev.settings.osr_h) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_OFF);
        }
        else if(BME280_OVERSAMPLING_1X == copy_of_sensorDev.settings.osr_h) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_1X);
        }
        else if(BME280_OVERSAMPLING_2X == copy_of_sensorDev.settings.osr_h) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_2X);
        }
        else if(BME280_OVERSAMPLING_4X == copy_of_sensorDev.settings.osr_h) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_4X);
        }
        else if(BME280_OVERSAMPLING_8X == copy_of_sensorDev.settings.osr_h) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_8X);
        }    
        else if(BME280_OVERSAMPLING_16X == copy_of_sensorDev.settings.osr_h) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_16X);
        }
        else {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_ERR);
        }
    }
    else {
        
        /* Humidity config status off */
        strcpy(envVarConfMsg, RES_STR_SCONF_OFF);
    }
    
    /* Send humidity config */
    len = send(clientSocket, &envVarConfMsg, sizeof(envVarConfMsg), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send temperature config to client */
#ifdef SERVER_DEBUG
        perror("send");
        fprintf(stderr, LOG_SYS_ERR_SERVER_CONF_HUM_SEND_FAIL, user->name);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_CONF_HUM_SEND_FAIL, user->name);
        
        error = -1;
        return error;
    }
    
    /* Parse and send pressure config */
    
    memset(envVarConfMsg, 0, sizeof(envVarConfMsg)); // Reset shared variable
    
    /* Parse pressure config */
    if(copy_of_sensorSettingSel & BME280_OSR_PRESS_SEL) {
        
        /* Pressure config status on */
        
        if(BME280_NO_OVERSAMPLING == copy_of_sensorDev.settings.osr_p) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_OFF);
        }
        else if(BME280_OVERSAMPLING_1X == copy_of_sensorDev.settings.osr_p) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_1X);
        }
        else if(BME280_OVERSAMPLING_2X == copy_of_sensorDev.settings.osr_p) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_2X);
        }
        else if(BME280_OVERSAMPLING_4X == copy_of_sensorDev.settings.osr_p) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_4X);
        }
        else if(BME280_OVERSAMPLING_8X == copy_of_sensorDev.settings.osr_p) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_8X);
        }   
        else if(BME280_OVERSAMPLING_16X == copy_of_sensorDev.settings.osr_p) {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_OVERSAMPLING_16X);
        }
        else {
            
            strcpy(envVarConfMsg, RES_STR_SCONF_ERR);
        }
    }
    else {
        
        /* Pressure config status off */
        strcpy(envVarConfMsg, RES_STR_SCONF_OFF);
    }
    
    /* Send pressure config */
    len = send(clientSocket, &envVarConfMsg, sizeof(envVarConfMsg), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send pressure config to client */
#ifdef SERVER_DEBUG
        perror("send");
        fprintf(stderr, LOG_SYS_ERR_SERVER_CONF_PRS_SEND_FAIL, user->name);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_CONF_PRS_SEND_FAIL, user->name);
        
        error = -1;
        return error;
    }
    
    /* Parse and send IIR filter config */
    
    /* Parse IIR filter config */
    if(copy_of_sensorSettingSel & BME280_FILTER_SEL) {
        
        /* IIR filter config status on */
        
        if(BME280_FILTER_COEFF_OFF == copy_of_sensorDev.settings.filter) {
            
            strcpy(iirFltrConfMsg, RES_STR_SCONF_IIR_COEFF_OFF);
        }
        else if(BME280_FILTER_COEFF_2 == copy_of_sensorDev.settings.filter) {
            
            strcpy(iirFltrConfMsg, RES_STR_SCONF_IIR_COEFF_2);
        }
        else if(BME280_FILTER_COEFF_4 == copy_of_sensorDev.settings.filter) {
            
            strcpy(iirFltrConfMsg, RES_STR_SCONF_IIR_COEFF_4);
        }
        else if(BME280_FILTER_COEFF_8 == copy_of_sensorDev.settings.filter) {
            
            strcpy(iirFltrConfMsg, RES_STR_SCONF_IIR_COEFF_8);
        }
        else if(BME280_FILTER_COEFF_16 == copy_of_sensorDev.settings.filter) {
            
            strcpy(iirFltrConfMsg, RES_STR_SCONF_IIR_COEFF_16);
        }
        else {
            
            strcpy(iirFltrConfMsg, RES_STR_SCONF_ERR);
        }
    }
    else {
        
        /* IIR fitter config status off */
        strcpy(iirFltrConfMsg, RES_STR_SCONF_OFF);
    }
    
    /* Send IIR filter config */
    len = send(clientSocket, &iirFltrConfMsg, sizeof(iirFltrConfMsg), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send IIR filter config to client */
#ifdef SERVER_DEBUG
        perror("send");
        fprintf(stderr, LOG_SYS_ERR_SERVER_CONF_IIR_SEND_FAIL, user->name);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_CONF_IIR_SEND_FAIL, user->name);
        
        error = -1;
        return error;
    }
    
    // PERIOD   <VAL>
    // TEMP     <X|OS_OFF|OS_1X|OS_2X|OS_4X|OS_8X|OS_16X>
    // HUM                      -,,-
    // PRESS                    -,,-
    // IIR      <X|COEFF_OFF|COEFF_2|COEFF_4|COEFF_8|COEFF_16>
    
    /* Syslog successful config data transmission */
#ifdef SERVER_DEBUG
    fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_GET_CONF_SUCCESS, user->name);
    fflush(stdout);
#endif
    syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_GET_CONF_SUCCESS, user->name);
    
    return error;
}

/*
 * Function 'removeDataHandler': removes measurement data requested by client.
 * 
 * Protocol:    Client --> Server: remove measurement data request code <1 byte>
 *              Client <-- Server: result of request code validation <1 byte>
 * 
 *              ++ In case of successful request validation ++
 * 
 *              Client <-- Server: result of request processing <1 byte>
 */
int removeDataHandler(int clientSocket, const struct UserData *user, void *customArg) {
    
    uint8_t response = 0;
    int error = 0;
    int len;
    
    /* Syslog client requested to remove measurement data */
#ifdef SERVER_DEBUG
    fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_RMV_DATA, user->name);
    fflush(stdout);
#endif
    syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_RMV_DATA, user->name);
    
    /* Check if file is open (= has content) */
    
    pthread_mutex_lock(&savedDataMutex);
    
    if(savedDataFd < 0) {
        
        /* File is closed (does not have content) */
        
        /* Nothing to do */
        pthread_mutex_unlock(&savedDataMutex);
    }
    else {
        
        /* File is open (does have content) */
        
        /* Remove content of file by reopening it */
    
        /* Close file */
        if(close(savedDataFd) < 0) {
            
#ifdef SERVER_DEBUG
            perror("close");
            fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_CLOSE_FAIL);
            fprintf(stderr, LOG_SYS_ERR_SERVER_RMV_DATA_FAIL, user->name);
            fflush(stderr);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_CLOSE_FAIL);
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_RMV_DATA_FAIL, user->name);
            
            pthread_mutex_unlock(&savedDataMutex);
            error = -1;
            return error;
        }
        
        savedDataFd = SAVED_DATA_FD_INVALID;
        
        /* Open file */
        savedDataFd = open(savedDataFilePath, O_CREAT | O_TRUNC | O_RDWR, 0644);
        if(savedDataFd < 0) {
                    
#ifdef SERVER_DEBUG
            perror("open");
            fprintf(stderr, LOG_SYS_ERR_SERVER_SAVE_OPEN_FAIL);
            fprintf(stderr, LOG_SYS_ERR_SERVER_RMV_DATA_FAIL, user->name);
            fflush(stderr);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_SAVE_OPEN_FAIL);
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_RMV_DATA_FAIL, user->name);
            
            pthread_mutex_unlock(&savedDataMutex);
            error = -1;
            return error;
        }
        
        /* At this point the content of the file is removed */
        
        pthread_mutex_unlock(&savedDataMutex);
    }
    
    /* Syslog success */
#ifdef SERVER_DEBUG
    fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_RMV_DATA_SUCCESS, user->name);
    fflush(stdout);
#endif
    syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_RMV_DATA_SUCCESS, user->name);
    
    /* Notify client on success */
    response = RES_CODE_REQ_SUCCESS;
    len = send(clientSocket, &response, sizeof(response), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send server response to client */
#ifdef SERVER_DEBUG
        perror("send");
        fprintf(stderr, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_RES_FAIL, user->name, response);
        error = -1;
    }

    return error;
}

/*
 * Function 'getDataHandler': returns measurement data requested by client.
 * 
 * Protocol:    Client --> Server: transfer measurement data request code <1 byte>
 *              Client <-- Server: result of request code validation <1 byte>
 * 
 *              ++ In case of successful request validation ++
 * 
 *              Client <-- Server: file size <4 bytes>
 *              Client <-- Server: file content <<file size> bytes>  (if not empty)
 */
int getDataHandler(int clientSocket, const struct UserData *user, void *customArg) {
    
    uint8_t response = 0;           // Not used
    int error = 0;                  // Error indicator
    int len;
    int fileSize = 0;               // Measurement data file size
    
    off_t fileOffset = 0;
    struct stat fileStat;
    
    /* Syslog client requested to get measurement data */
#ifdef SERVER_DEBUG
    fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_GET_DATA, user->name);
    fflush(stdout);
#endif
    syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_GET_DATA, user->name);
    
    /* Check if saved data file is open */
    
    pthread_mutex_lock(&savedDataMutex);
    
    if(savedDataFd < 0) {
     
        /* Saved data file is closed */
#ifdef SERVER_DEBUG
        fprintf(stderr, LOG_SYS_ERR_SERVER_FCONT_ACCESS_FAIL, user->name);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_FCONT_ACCESS_FAIL, user->name);
        
        // TODO Enhance security by sending client an invalid size
        
        pthread_mutex_unlock(&savedDataMutex);
        error = -1;
        return error;
    }
    
    /* Get file stat including file size */
    if(0 > fstat(savedDataFd, &fileStat)) {
        
        /* Failed to get file stat */
#ifdef SERVER_DEBUG
        perror("fstat");
        fprintf(stderr, LOG_SYS_ERR_SERVER_FSTAT_GET_FAIL);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_FSTAT_GET_FAIL);
        
        // TODO Enhance security by sending client an invalid size
        
        pthread_mutex_unlock(&savedDataMutex);
        error = -1;
        return error;
    }
    
    fileSize = fileStat.st_size;
    
    /* Send file size to client */
    len = send(clientSocket, &fileSize, sizeof(fileSize), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send file size to client */
#ifdef SERVER_DEBUG
        perror("send");
        fprintf(stderr, LOG_SYS_ERR_SERVER_FSIZE_SEND_FAIL, user->name);
        fflush(stderr);
#endif
        syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_FSIZE_SEND_FAIL, user->name);
        
        pthread_mutex_unlock(&savedDataMutex);
        error = -1;
        return error;
    }
    
    /* If file size is 0 kB do not send anything */
    /* Client will receive size and do so */
    
    if(0 != fileSize) {
        
        /* Send file content to client */
        len = sendfile(clientSocket, savedDataFd, &fileOffset, fileSize);
        if(len < 0) {
         
            /* Failed to send file content to client */
#ifdef SERVER_DEBUG
            perror("sendfile");
            fprintf(stderr, LOG_SYS_ERR_SERVER_FCONT_SEND_FAIL, user->name);
            fflush(stderr);
#endif
            syslog(LOG_DAEMON | LOG_ERR, LOG_SYS_ERR_SERVER_FCONT_SEND_FAIL, user->name);
            
            pthread_mutex_unlock(&savedDataMutex);
            error = len;
            return error;
        }
    }
    
    pthread_mutex_unlock(&savedDataMutex);
    
    /* Syslog successful data transfer */
    if(0 == error) {
        
#ifdef SERVER_DEBUG
        fprintf(stdout, LOG_SYS_INFO_CLIENT_REQ_GET_DATA_SUCCESS, user->name);
        fflush(stdout);
#endif
        syslog(LOG_DAEMON | LOG_INFO, LOG_SYS_INFO_CLIENT_REQ_GET_DATA_SUCCESS, user->name);
    }
    
    return error;
}
