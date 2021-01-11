/*
 * FileName:    client.c
 * Author:      Adam Csizy
 * Neptun Code: ******
 * 
 * Desc.:       Source file containing function definitions which implement
 *              client request handling and client-server communication.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"

/* Function definitions */

/*
 * Function 'initMeasDataFilePath': initializes file path for file containing measurement data.
 */
int initMeasDataFilePath(char filePathBuf[]) {
    
    int error = 0;
    char *ret = NULL;
    
    ret = getcwd(filePathBuf, MEAS_DATA_FILE_PATH_LEN);
    if(NULL == ret) {
        
        perror("getcwd");
        fprintf(stderr, MSG_USER_WARN_MEAS_PATH_INIT_FAIL);
        fflush(stderr);
        
        error = -1;
        return error;
    }
    
    strcat(filePathBuf, "/");
    strcat(filePathBuf, MEAS_DATA_FILE_NAME);
    
    return error;
}

/*
 * Function 'connectServer': connects to remote server.
 */
int connectServer(const char *ipAddress, const char *portNumber, struct pollfd pollArray[]) {
    
    uint8_t response;
    int error;
    int len;
    int keepAliveState = 1;
    char username[USR_NAME_MAX_LENGTH];
    char password[USR_PWD_MAX_LENGTH];
    
    struct addrinfo hints;
    struct addrinfo *res;
    
    /* Initialize user data */
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));
    
    /* Check if there is no active connection yet */
    if(INVALID_FD != pollArray[POLL_ARRAY_SOCKET].fd) {
        
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL_EXIST);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        return -1;
    }
    
    /* Check if function arguments are valid */
    if((NULL == ipAddress) || (NULL == portNumber)) {
        
        fprintf(stdout, MSG_USER_WARN_INVALID_ARG);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    
    /* Initialize hints structure with TCP and either IPv4 or IPv6 address */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    /* Resolve server address */
    error = getaddrinfo(ipAddress, portNumber, &hints, &res);
    if(0 != error) {
        
        /* Failed to resolve server address */
        
        fprintf(stderr, "[ERROR] getaddrinfo: %s\n", gai_strerror(error));
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL_ADDR);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    else if(NULL == res) {
        
        /* No server found with the given parameters */
        
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL_ADDR);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    
    /* Create client socket based on res settings */
    pollArray[POLL_ARRAY_SOCKET].fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(pollArray[POLL_ARRAY_SOCKET].fd < 0) {
        
        /* Failed to open client socket */
        
        perror("socket");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL_SOCK);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        /* Free address-info list pointed by res */
        freeaddrinfo(res);
        
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    
    /* Connect client socket to server socket referenced to by ai_addr */
    if(connect(pollArray[POLL_ARRAY_SOCKET].fd, res->ai_addr, res->ai_addrlen) < 0) {
        
        /* Failed to connect to server socket */
        
        perror("connect");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        /* Free address-info list pointed by res */
        freeaddrinfo(res);
        
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    
    /* Authenticate client */
    
    /* Read username */
    fprintf(stdout, MSG_USER_REQ_NAME);
    fflush(stdout);
    if((len = read(pollArray[POLL_ARRAY_STDIN].fd, username, sizeof(username))) < 0) {
        
        /* Failed to read username */
        perror("read");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_REQ_NAME_FAIL);
        fflush(stdout);
        
        /* Send empty username to server */
    }
    else {
     
        /* Remove the new line character */
        username[strlen(username)-1] = '\0';
    }
    
    /* Read password */
    fprintf(stdout, MSG_USER_REQ_PASSWORD);
    fflush(stdout);
    if((len = read(pollArray[POLL_ARRAY_STDIN].fd, password, sizeof(password))) < 0) {
        
        /* Failed to read password */
        perror("read");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_REQ_PWD_FAIL);
        fflush(stdout);
        
        /* Send empty password to server */
    }
    else {
        
        /* Remove the new line character */
        password[strlen(password)-1] = '\0';
    }
    
    len = send(pollArray[POLL_ARRAY_SOCKET].fd, username, sizeof(username), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send username to server */
        perror("send");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_SEND_NAME_FAIL);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        /* Free address-info list pointed by res */
        freeaddrinfo(res);
        
        close(pollArray[POLL_ARRAY_SOCKET].fd);
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    
    len = send(pollArray[POLL_ARRAY_SOCKET].fd, password, sizeof(password), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send password to server */
        perror("send");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_SEND_PWD_FAIL);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        /* Free address-info list pointed by res */
        freeaddrinfo(res);
        
        close(pollArray[POLL_ARRAY_SOCKET].fd);
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &response, sizeof(response), MSG_WAITALL);
    if(len < 0) {
        
        /* Failed to receive authentication response from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RECV_AUTH_FAIL);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        /* Free address-info list pointed by res */
        freeaddrinfo(res);
        
        close(pollArray[POLL_ARRAY_SOCKET].fd);
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    
    if(RES_CODE_AUTH_SUCCESS != response){
        
        /* Authentication failed */
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL_AUTH);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        /* Free address-info list pointed by res */
        freeaddrinfo(res);
        
        close(pollArray[POLL_ARRAY_SOCKET].fd);
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    
    /* Set socket option SO_KEEPALIVE for enhanced safety */
    if(setsockopt(pollArray[POLL_ARRAY_SOCKET].fd, SOL_SOCKET, SO_KEEPALIVE, &keepAliveState, sizeof(keepAliveState)) < 0) {
        
        /* Failed to set socket option SO_KEEPALIVE */
        
        perror("setsockopt");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_CONNECT_FAIL);
        fflush(stdout);
        
        /* Free address-info list pointed by res */
        freeaddrinfo(res);
        
        close(pollArray[POLL_ARRAY_SOCKET].fd);
        pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
        return -1;
    }
    
    /* Free address-info list pointed by res */
    freeaddrinfo(res);
    
    /* Connection was successfully estabilished */
    fprintf(stdout, MSG_USER_INFO_CONNECT_SUCCESS);
    fflush(stdout);
    
    return 0;
}

/* 
 * Function 'disconnectServer': disconnects from the remote server.
 * 
 * Protocol:    Client --> Server: disconnect request code <1 byte>
 */
int disconnectServer(struct pollfd pollArray[]) {
    
    uint8_t request;
    int len;
    
    /* Check if there is an active connection */
    if(INVALID_FD == pollArray[POLL_ARRAY_SOCKET].fd) {
        
        /* Nothing to do */
        fprintf(stdout, MSG_USER_INFO_DISCONNECT_NONE);
        fflush(stdout);
        
        return 0;
    }
    
    /* Send close message to server */
    request = REQ_CODE_DCONN;
    len = send(pollArray[POLL_ARRAY_SOCKET].fd, &request, sizeof(request), MSG_NOSIGNAL);
    if(len < 0) {
     
        /* Failed to send disconnect message to server */
        perror("close");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_DISCONNECT_FAIL_REQ);
        fflush(stdout);
    }
    
    /* Close client socket */
    if(close(pollArray[POLL_ARRAY_SOCKET].fd) < 0) {
        
        /* Failed to close client socket */
        perror("close");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_DISCONNECT_FAIL_SOCK);
        fprintf(stdout, MSG_USER_WARN_DISCONNECT_FAIL);
        fflush(stdout);

        return -1;
    }
    
    /* Connection was successfully closed */
    fprintf(stdout, MSG_USER_INFO_DISCONNECT);
    fflush(stdout);
    
    pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
    return 0;
}

/*
 * Function 'exitProgram': implements exit preparations.
 */
int exitProgram(struct pollfd pollArray[]) {
    
    /* Check if there is an active connection */
    if(INVALID_FD != pollArray[POLL_ARRAY_SOCKET].fd) {
        
        /* Close active connection before exiting */
        if(disconnectServer(pollArray) < 0) {
            
            /* Let the OS fix it after terminating the program */
            
            /* Set exit condition */
            exitCondition = COND_EXIT_TRUE;
            
            return -1;
        }
    }
    
    /* Set exit condition */
    exitCondition = COND_EXIT_TRUE;
    
    return 0;
}

/*
 * Function 'setSensorConfig': requests server to set sensor configuration.
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
int setSensorConfig(struct pollfd pollArray[], const char* args[]) {
    
    uint8_t request = 0x00;        // Request code (from client to server)
    uint8_t configSel = 0x00;      // Configuration selector
    uint8_t response = 0x00;       // Response code (from server to client)
    int period = 0;
    int error = 0;
    int len;
    
    /* Check if there is an active connection */
    if(INVALID_FD == pollArray[POLL_ARRAY_SOCKET].fd) {
        
        /* No active connection */
        fprintf(stdout, MSG_USER_WARN_NO_CONN);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Check NULL of config type argument */
    if(NULL == args[1]) {
        
        /* Invalid config type argument */
        fprintf(stdout, MSG_USER_WARN_INVALID_CONF_TYPE_ARG);
        fprintf(stdout, MSG_USER_INFO_HINT_CONF);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Interpret user command */
    if(0 == strcmp(args[1], STR_CMD_SCONF_PERIOD)) {
        
        /* Request period config */
        
        configSel |= REQ_CONF_PRD;
        
        /* Check NULL of config period argument */
        if(NULL == args[2]) {
            
            fprintf(stdout, MSG_USER_WARN_INVALID_CONF_PRD_ARG);
            fprintf(stdout, MSG_USER_INFO_HINT_CONF);
            fflush(stdout);
            
            error = -1;
            return error;
        }
        
        /* Convert period from string to integer */
        period = atoi(args[2]);
    }
    else if(0 == strcmp(args[1], STR_CMD_SCONF_IIR_FILTER)) {
        
        /* Request IIR filter config */
        
        configSel |= REQ_CONF_IIR;
        
        /* Check NULL of config status argument */
        if(NULL == args[2]) {
            
            fprintf(stdout, MSG_USER_WARN_INVALID_CONF_STAT_ARG);
            fprintf(stdout, MSG_USER_INFO_HINT_CONF);
            fflush(stdout);
            
            error = -1;
            return error;
        }
        
        /* Identify config status */
        if(0 == strcmp(args[2], STR_CMD_SCONF_ON)) {
            
            /* Config status on */
            configSel |= REQ_CONF_ON;
            
            /* Check NULL of config value argument */
            if(NULL == args[3]) {
            
                fprintf(stdout, MSG_USER_WARN_INVALID_CONF_VAL_ARG);
                fprintf(stdout, MSG_USER_INFO_HINT_CONF);
                fflush(stdout);
            
                error = -1;
                return error;
            }
            
            /* Identify config value */
            if(0 == strcmp(args[3], STR_CMD_SCONF_IIR_COEFF_OFF)) {
                
                /* Filter coefficient off */
                configSel |= REQ_CONF_COEFF_OFF;
            }
            else if(0 == strcmp(args[3], STR_CMD_SCONF_IIR_COEFF_2)) {
                
                /* Filter coefficient 2 */
                configSel |= REQ_CONF_COEFF_2;
            }
            else if(0 == strcmp(args[3], STR_CMD_SCONF_IIR_COEFF_4)) {
                
                /* Filter coefficient 4 */
                configSel |= REQ_CONF_COEFF_4;
            }
            else if(0 == strcmp(args[3], STR_CMD_SCONF_IIR_COEFF_8)) {
                
                /* Filter coefficient 8 */
                configSel |= REQ_CONF_COEFF_8;
            }
            else if(0 == strcmp(args[3], STR_CMD_SCONF_IIR_COEFF_16)) {
                
                /* Filter coefficient 16 */
                configSel |= REQ_CONF_COEFF_16;
            }
            else {
                
                /* Invalid config value argument */
                fprintf(stdout, MSG_USER_WARN_INVALID_CONF_VAL_ARG);
                fprintf(stdout, MSG_USER_INFO_HINT_CONF);
                fflush(stdout);
                
                error = -1;
                return error;
            }
        }
        else if(0 == strcmp(args[2], STR_CMD_SCONF_OFF)) {
            
            /* Config status off */
            configSel |= REQ_CONF_OFF;
        }
        else {
            
            /* Invalid config status argument */
            fprintf(stdout, MSG_USER_WARN_INVALID_CONF_STAT_ARG);
            fprintf(stdout, MSG_USER_INFO_HINT_CONF);
            fflush(stdout);
            
            error = -1;
            return error;
        }
    }
    else if(
        
        (0 == strcmp(args[1], STR_CMD_SCONF_HUMIDITY)) ||
        (0 == strcmp(args[1], STR_CMD_SCONF_PRESSURE)) ||
        (0 == strcmp(args[1], STR_CMD_SCONF_TEMPERATURE))
    ) {
        
        /* Request humidity/pressure/temperature config */
        /* The interpretation of these configs are identical */
        
        /* Identify config type */
        if(0 == strcmp(args[1], STR_CMD_SCONF_HUMIDITY)) {
            
            /* Request humidity config */
            configSel |= REQ_CONF_HUM;
        }
        else if(0 == strcmp(args[1], STR_CMD_SCONF_PRESSURE)) {
            
            /* Request pressure config */
            configSel |= REQ_CONF_PRS;
        }
        else{
         
            /* Request temperature config */
            configSel |= REQ_CONF_TMP;
        }
        
        /* Check NULL of config status argument */
        if(NULL == args[2]) {
            
            fprintf(stdout, MSG_USER_WARN_INVALID_CONF_STAT_ARG);
            fprintf(stdout, MSG_USER_INFO_HINT_CONF);
            fflush(stdout);
            
            error = -1;
            return error;
        }
        
        /* Identify config status */
        if(0 == strcmp(args[2], STR_CMD_SCONF_ON)){
            
            /* Config status on */
            configSel |= REQ_CONF_ON;
            
            /* Check NULL of config value argument */
            if(NULL == args[3]) {
            
                fprintf(stdout, MSG_USER_WARN_INVALID_CONF_VAL_ARG);
                fprintf(stdout, MSG_USER_INFO_HINT_CONF);
                fflush(stdout);
            
                error = -1;
                return error;
            }
            
            /* Identify config value */
            if(0 == strcmp(args[3], STR_CMD_SCONF_OVERSAMPLING_OFF)) {
            
                /* Oversmapling off */
                configSel |= REQ_CONF_OS_OFF;
            }
            else if(0 == strcmp(args[3], STR_CMD_SCONF_OVERSAMPLING_1X)) {
            
                /* Oversampling 1X */
                configSel |= REQ_CONF_OS_1X;
            }
            else if(0 == strcmp(args[3], STR_CMD_SCONF_OVERSAMPLING_2X)) {
            
                /* Oversampling 2X */
                configSel |= REQ_CONF_OS_2X;
            }
            else if(0 == strcmp(args[3], STR_CMD_SCONF_OVERSAMPLING_4X)) {
            
                /* Oversampling 4X */
                configSel |= REQ_CONF_OS_4X;
            }
            else if(0 == strcmp(args[3], STR_CMD_SCONF_OVERSAMPLING_8X)) {
            
                /* Oversampling 8X */
                configSel |= REQ_CONF_OS_8X;
            }
            else if(0 == strcmp(args[3], STR_CMD_SCONF_OVERSAMPLING_16X)) {
            
                /* Oversampling 16X */
                configSel |= REQ_CONF_OS_16X;
            }
            else {
            
                /* Invalid config value argument */
                fprintf(stdout, MSG_USER_WARN_INVALID_CONF_VAL_ARG);
                fprintf(stdout, MSG_USER_INFO_HINT_CONF);
                fflush(stdout);
                
                error = -1;
                return error;
            }
            
        }
        else if(0 == strcmp(args[2], STR_CMD_SCONF_OFF)) {
            
            /* Config status off */
            configSel |= REQ_CONF_OFF;
        }
        else {
            
            /* Invalid config status argument */
            fprintf(stdout, MSG_USER_WARN_INVALID_CONF_STAT_ARG);
            fprintf(stdout, MSG_USER_INFO_HINT_CONF);
            fflush(stdout);
            
            error = -1;
            return error;
        }
    }
    else {
     
        /* Invalid config type argument */
        fprintf(stdout, MSG_USER_WARN_INVALID_CONF_TYPE_ARG);
        fprintf(stdout, MSG_USER_INFO_HINT_CONF);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Note: at this point variable 'configSel' is fully configured */
    
    /* Set request to 'sconf' */
    request = REQ_CODE_SCONF;
    
    /* Note: at this point the whole request message is fully configured */
    
    // Period: server << request(uint8_t) << configSel(uint8_t) << period(int)
    // Others: server << request(uint8_t) << configSel(uint8_t)
        
    /* Send request code to server */
    len = send(pollArray[POLL_ARRAY_SOCKET].fd, &request, sizeof(request), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send client request to server */
        perror("send");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_SEND_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
        
    /* Receive server response about the outcome of processing the reqest code */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &response, sizeof(response), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive response from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RES_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Process server response */
    if(RES_CODE_REQ_ACCEPT == response) {
        
        /* Client request accepted */
    }
    else if(RES_CODE_REQ_NO_PERM == response) {
        
        /* Client does not have permission for the issued request */
        fprintf(stdout, MSG_USER_WARN_REQ_NO_PERM);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    else if(RES_CODE_REQ_INVALID == response) {
        
        /* Client request was invalid */
        fprintf(stdout, MSG_USER_WARN_REQ_INVALID);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    else {
     
        /* Invalid server response */
        fprintf(stdout, MSG_USER_WARN_RES_INVALID);
        fflush(stdout);
        
        error = -1;
        return error;
    }
        
    /* Send config code to server */
    len = send(pollArray[POLL_ARRAY_SOCKET].fd, &configSel, sizeof(configSel), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send client request to server */
        perror("send");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_SEND_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Send period value based on config type (PRD) */
    if(REQ_CONF_PRD == (configSel & REQ_CONF_TYPE_MASK)) {
        
        /* Send period value */
        len = send(pollArray[POLL_ARRAY_SOCKET].fd, &period, sizeof(period), MSG_NOSIGNAL);
        if(len < 0) {
        
            /* Failed to send period value to server */
            perror("send");
            fflush(stderr);
            fprintf(stdout, MSG_USER_WARN_SEND_REQ_FAIL);
            fflush(stdout);
        
            error = -1;
            return error;
        }
    }
    
    /* Receive server response about the outcome of the remote configuration */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &response, sizeof(response), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive response from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RES_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Process server response */
    if(RES_CODE_REQ_SUCCESS == response) {
        
        /* Client configuration request succeeded */
        fprintf(stdout, MSG_USER_INFO_REQ_SUCCESS);
        fflush(stdout);
    }
    else if(RES_CODE_REQ_FAIL == response) {
        
        /* Client configuration request failed due to server side error */
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL_SERVER);
        fflush(stdout);
    }
    else {
        
        /* Invalid server response */
        fprintf(stdout, MSG_USER_WARN_RES_INVALID);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    return error;
}

/*
 * Function 'getSensorConfig': requests server to get sensor configuration.
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
int getSensorConfig(struct pollfd pollArray[]) {
    
    uint8_t request = 0;
    uint8_t response = 0;
    int error = 0;
    int len;
    
    int measPeriod = 0;                         // Measurement period [sec]
    char envVarConfMsg[8];                      // String message of TMP/HUM/PRS configs (collective)
    char iirFltrConfMsg[16];                    // String message of IIR filter config
    
    /* Initialize local variables */
    memset(envVarConfMsg, 0, sizeof(envVarConfMsg));
    memset(iirFltrConfMsg, 0, sizeof(iirFltrConfMsg));
    
    /* Check if there is an active connection */
    if(INVALID_FD == pollArray[POLL_ARRAY_SOCKET].fd) {
        
        /* No active connection */
        fprintf(stdout, MSG_USER_WARN_NO_CONN);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Send request code to server */
    request = REQ_CODE_GCONF;
    len = send(pollArray[POLL_ARRAY_SOCKET].fd, &request, sizeof(request), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send client request to server */
        perror("send");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_SEND_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Receive server response about the outcome of processing the reqest code */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &response, sizeof(response), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive response from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RES_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Process server response */
    if(RES_CODE_REQ_ACCEPT == response) {
        
        /* Client request accepted */
    }
    else if(RES_CODE_REQ_NO_PERM == response) {
        
        /* Client does not have permission for the issued request */
        fprintf(stdout, MSG_USER_WARN_REQ_NO_PERM);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    else if(RES_CODE_REQ_INVALID == response) {
        
        /* Client request was invalid */
        fprintf(stdout, MSG_USER_WARN_REQ_INVALID);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    else {
     
        /* Invalid server response */
        fprintf(stdout, MSG_USER_WARN_RES_INVALID);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Receive config data from server and print it out */
    
    /* Receive measurement period from server */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &measPeriod, sizeof(measPeriod), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive measurement period from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RECV_CONF_PRD_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Print measurement period */
    fprintf(stdout, MSG_USER_INFO_CONF_HEADER);
    fprintf(stdout, MSG_USER_INFO_CONF_PERIOD, measPeriod);
    fflush(stdout);
    
    
    /* Receive temperature config from server */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &envVarConfMsg, sizeof(envVarConfMsg), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive temperature config from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RECV_CONF_TMP_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Print temperature config */
    fprintf(stdout, MSG_USER_INFO_CONF_TEMPERATURE, envVarConfMsg);
    fflush(stdout);
    
    /* Receive humidity config from server */
    memset(envVarConfMsg, 0, sizeof(envVarConfMsg)); // Reset shared variable
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &envVarConfMsg, sizeof(envVarConfMsg), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive humidity config from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RECV_CONF_HUM_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Print humidity config */
    fprintf(stdout, MSG_USER_INFO_CONF_HUMIDITY, envVarConfMsg);
    fflush(stdout);
    
    /* Receive pressure config from server */
    memset(envVarConfMsg, 0, sizeof(envVarConfMsg)); // Reset shared variable
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &envVarConfMsg, sizeof(envVarConfMsg), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive pressure config from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RECV_CONF_PRS_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Print pressure config */
    fprintf(stdout, MSG_USER_INFO_CONF_PRESSURE, envVarConfMsg);
    fflush(stdout);
    
    /* Receive IIR filter config from server */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &iirFltrConfMsg, sizeof(iirFltrConfMsg), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive IIR filer config from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RECV_CONF_IIR_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Print IIR filter config */
    fprintf(stdout, MSG_USER_INFO_CONF_FILTER, iirFltrConfMsg);
    fprintf(stdout, MSG_USER_INFO_CONF_FOOTER);
    fflush(stdout);
    
    return error;
}

/*
 * Function 'getSensorData': requests server to get sensor data.
 * 
 * Protocol:    Client --> Server: transfer measurement data request code <1 byte>
 *              Client <-- Server: result of request code validation <1 byte>
 * 
 *              ++ In case of successful request validation ++
 * 
 *              Client <-- Server: file size <4 bytes>
 *              Client <-- Server: file content <<file size> bytes>  (if not empty)
 */
int getSensorData(struct pollfd pollArray[]) {
    
    uint8_t request = 0;
    uint8_t response = 0;
    int error = 0;
    int measDataFd = 0;
    int len;
    int dataFileSize = 0;
    float measData[3];
    
    /* Check if there is an active connection */
    if(INVALID_FD == pollArray[POLL_ARRAY_SOCKET].fd) {
        
        /* No active connection */
        fprintf(stdout, MSG_USER_WARN_NO_CONN);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Send request code to server */
    request = REQ_CODE_GDAT;
    len = send(pollArray[POLL_ARRAY_SOCKET].fd, &request, sizeof(request), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send client request to server */
        perror("send");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_SEND_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Receive server response about the outcome of processing the reqest code */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &response, sizeof(response), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive response from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RES_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Process server response */
    if(RES_CODE_REQ_ACCEPT == response) {
        
        /* Client request accepted */
    }
    else if(RES_CODE_REQ_NO_PERM == response) {
        
        /* Client does not have permission for the issued request */
        fprintf(stdout, MSG_USER_WARN_REQ_NO_PERM);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    else if(RES_CODE_REQ_INVALID == response) {
        
        /* Client request was invalid */
        fprintf(stdout, MSG_USER_WARN_REQ_INVALID);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    else {
     
        /* Invalid server response */
        fprintf(stdout, MSG_USER_WARN_RES_INVALID);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Receive measurement data file size (file size to be transmitted) */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &dataFileSize, sizeof(dataFileSize), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive measurement data file size */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RECV_FDATA_SIZE_FAIL);
        fprintf(stderr, MSG_USER_WARN_RECV_FDATA_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    if(dataFileSize < 0) {
        
        /* Invalid file size (server side error) */
        
        error = -1;
        return error;
    }
    else if(0 == dataFileSize) {
        
        /* Measurement data is not available on server */
        fprintf(stdout, MSG_USER_WARN_RECV_FDATA_NOT_AVL);
        fflush(stdout);
        
        return error;
    }

    /* Open local file to save measurement data stored on remote server */
    measDataFd = open(measDataFilePath, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if(measDataFd < 0) {
        
        /* Failed to open local measurement data file */
        perror("open");
        fprintf(stderr, MSG_USER_WARN_MEAS_FILE_OPEN_FAIL);
        fprintf(stderr, MSG_USER_WARN_RECV_FDATA_FAIL);
        fflush(stderr);

        error = -1;
        return error;
    }
    
    memset(measData, 0, sizeof(measData));
    
    /* Read from socket to local file */
    while(dataFileSize > 0) {
        
        len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &measData, sizeof(measData), MSG_WAITALL);
        /* Write temporary measurement data to local file */
        len = write(measDataFd, &measData, sizeof(measData));
        if(len < 0) {
            
            perror("write");
            fprintf(stderr, MSG_USER_WARN_MEAS_FILE_WRITE_FAIL);
            fflush(stderr);
            
            error = -1;
            /* Skip this sample and keep receiving data from server */
        }
        
        /* Clear temporary measurement data */
        memset(measData, 0, sizeof(measData));
        dataFileSize -= len;
    }
    
    /* Error check */
    if(len < 0) {
         
        /* Failed to receive measurement data from server */
        perror("recv");
        fprintf(stderr, MSG_USER_WARN_RECV_FDATA_FAIL);
        fflush(stderr);
            
        error = -1;
        return error;
    }
    
    /* Close file */
    close(measDataFd);
    
    if(0 == error) {
        
        fprintf(stdout, MSG_USER_INFO_RECV_FDATA_SUCCESS);
        fflush(stdout);
    }
    
    return error;
}

/*
 * Function 'removeSensorData': requests server to remove sensor data.
 * 
 * Protocol:    Client --> Server: remove measurement data request code <1 byte>
 *              Client <-- Server: result of request code validation <1 byte>
 * 
 *              ++ In case of successful request validation ++
 * 
 *              Client <-- Server: result of request processing <1 byte>
 */
int removeSensorData(struct pollfd pollArray[]) {
    
    uint8_t request = 0;
    uint8_t response = 0;
    int error = 0;
    int len;
    
    /* Check if there is an active connection */
    if(INVALID_FD == pollArray[POLL_ARRAY_SOCKET].fd) {
        
        /* No active connection */
        fprintf(stdout, MSG_USER_WARN_NO_CONN);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Send request code to server */
    request = REQ_CODE_RMDAT;
    len = send(pollArray[POLL_ARRAY_SOCKET].fd, &request, sizeof(request), MSG_NOSIGNAL);
    if(len < 0) {
        
        /* Failed to send client request to server */
        perror("send");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_SEND_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Receive server response about the outcome of processing the reqest code */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &response, sizeof(response), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive response from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RES_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Process server response */
    if(RES_CODE_REQ_ACCEPT == response) {
        
        /* Client request accepted */
    }
    else if(RES_CODE_REQ_NO_PERM == response) {
        
        /* Client does not have permission for the issued request */
        fprintf(stdout, MSG_USER_WARN_REQ_NO_PERM);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    else if(RES_CODE_REQ_INVALID == response) {
        
        /* Client request was invalid */
        fprintf(stdout, MSG_USER_WARN_REQ_INVALID);
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    else {
     
        /* Invalid server response */
        fprintf(stdout, MSG_USER_WARN_RES_INVALID);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Receive server response about the outcome of measurement data removal */
    len = recv(pollArray[POLL_ARRAY_SOCKET].fd, &response, sizeof(response), MSG_WAITALL);
    if(len < 0) {
     
        /* Failed to receive response from server */
        perror("recv");
        fflush(stderr);
        fprintf(stdout, MSG_USER_WARN_RES_FAIL);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    /* Process server response */
    if(RES_CODE_REQ_SUCCESS == response) {
        
        /* Data removal request succeeded */
        fprintf(stdout, MSG_USER_INFO_REQ_SUCCESS);
        fflush(stdout);
    }
    else if(RES_CODE_REQ_FAIL == response) {
        
        /* Data removal request failed due to server side error */
        fprintf(stdout, MSG_USER_WARN_REQ_FAIL_SERVER);
        fflush(stdout);
    }
    else {
        
        /* Invalid server response */
        fprintf(stdout, MSG_USER_WARN_RES_INVALID);
        fflush(stdout);
        
        error = -1;
        return error;
    }
    
    return error;
}

/*
 * Function 'showSensorData': show measurement data records transferred from server.
 */
int showSensorData(struct pollfd pollArray[]) {
 
    int dataFd = -1;
    int error = 0;
    float dataRecord[3];
    int len;
    
    /* Open local file to read and print measurement data records */
    dataFd = open(measDataFilePath, O_RDONLY | O_NONBLOCK);
    if(dataFd < 0) {
        
        /* Failed to open local measurement data file */
        perror("open");
        fprintf(stderr, "[ERROR] Failed to open measurement data file.\n");
        fprintf(stderr, "[ERROR] Failed to show measurement data records.\n");
        fflush(stderr);

        error = -1;
        return error;
    }
    
    /* Reset data record */
    memset(dataRecord, 0, sizeof(dataRecord));
    
    fprintf(stdout, "\n------------- SENSOR DATA RECORDS ------------\n\n");
    fflush(stdout);
    
    len = read(dataFd, &dataRecord, sizeof(dataRecord));
    while(len > 0) {
        
        fprintf(stdout, "  %0.2lf deg C       %0.2lf%%       %0.2lf hPa\n", dataRecord[0], dataRecord[1], dataRecord[2]);
        
        memset(dataRecord, 0, sizeof(dataRecord));
        len = read(dataFd, &dataRecord, sizeof(dataRecord));
    }
        
    fprintf(stdout, "\n-----------------------------------------------\n");
    fflush(stdout);
    
    return error;
}

/* 
 * Function 'interpretUserCommand': parse and interprets the command line user isntructions.
 */
int interpretUserCommand(struct pollfd pollArray[]) {
    
    const char* args[MAX_NUM_OF_ARGS];
    const char delim[] = " ";
    char inputBuffer[INPUT_BUFFER_SIZE];
    int argIndex = 0;
    int len;
    
    /* Initialize buffers for the sake of security */
    /* (Retentive memory issues from previous calls experienced) */
    memset(inputBuffer, 0, sizeof(inputBuffer));
    memset(args, 0, sizeof(args));
    
    /* Read user command */
    if((len = read(pollArray[POLL_ARRAY_STDIN].fd, inputBuffer, INPUT_BUFFER_SIZE)) < 0) {
        
        perror("read");
        fflush(stderr);
        return -1;
    }
    
    /* Remove the new line character in order to support single word commands */
    inputBuffer[strlen(inputBuffer)-1] = '\0';
    
    /* Parse user command */
    args[argIndex] = strtok(inputBuffer, delim);
    while((NULL != args[argIndex]) && ((MAX_NUM_OF_ARGS-1) > argIndex)) {
     
        argIndex++;
        args[argIndex] = strtok(NULL, delim);
    }
    
    /* Some debug code */
    //for(argIndex = 0; argIndex < MAX_NUM_OF_ARGS; argIndex++){
    // 
    //    fprintf(stdout, "[DEBUG] args[%d]: %s\n", argIndex, args[argIndex]);
    //    fflush(stdout);
    //}
    
    /* Check NULL of user command */
    if(NULL == args[0]) {
     
        fprintf(stdout, MSG_USER_WARN_INVALID_CMD);
        fflush(stdout);
        
        return -1;
    }
    
    /* Interpret user command */
    if(0 == strcmp(args[0], STR_CMD_SET_CONFIG)) {
        
        /* SET SENSOR CONFIGURATION */
        fprintf(stdout, "[INFO] SET CONFIG\n");
        fflush(stdout);
        setSensorConfig(pollArray, args);
    }
    else if(0 == strcmp(args[0], STR_CMD_GET_CONFIG)) {
        
        /* GET SENSOR CONFIGURATION */
        fprintf(stdout, "[INFO] GET CONFIG\n");
        fflush(stdout);
        getSensorConfig(pollArray);
    }
    else if(0 == strcmp(args[0], STR_CMD_GET_DATA)) {
        
        /* GET SENSOR DATA */
        fprintf(stdout, "[INFO] GET DATA\n");
        fflush(stdout);
        getSensorData(pollArray);
    }
    else if(0 == strcmp(args[0], STR_CMD_REMOVE_DATA)) {
        
        /* REMOVE SENSOR DATA */
        fprintf(stdout, "[INFO] REMOVE DATA\n");
        fflush(stdout);
        removeSensorData(pollArray);
    }
    else if(0 == strcmp(args[0], STR_CMD_DISCONNECT)) {
        
        /* DISCONNECT FROM SERVER */
        
        fprintf(stdout, "[INFO] DISCONNECT\n");
        fflush(stdout);
        disconnectServer(pollArray);
    }
    else if(0 == strcmp(args[0], STR_CMD_CONNECT)) {
        
        /* CONNECT TO SERVER */
        
        fprintf(stdout, "[INFO] CONNECT\n");
        fflush(stdout);
        connectServer(args[1], args[2], pollArray);
    }
    else if(0 == strcmp(args[0], STR_CMD_SHOW_DATA)) {
        
        /* SHOW MEASUREMENT DATA */
        
        fprintf(stdout, "[INFO] SHOW\n");
        fflush(stdout);
        showSensorData(pollArray);
    }
    else if(0 == strcmp(args[0], STR_CMD_EXIT)) {
        
        /* EXIT PROGRAM */
        
        fprintf(stdout, "[INFO] EXIT PROGRAM\n");
        fflush(stdout);
        exitProgram(pollArray);
    }
    else {
        
        /* INVALID USER COMMAND */
        
        fprintf(stdout, MSG_USER_WARN_INVALID_CMD);
        fflush(stdout);
    }
    
    return 0;
}
