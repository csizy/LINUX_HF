/*
 * FileName:    mysensor.c
 * Author:      Adam Csizy
 * Neptun Code: ******
 * 
 * Desc.:       Main thread of the client application.
 * 
 * Compile like this:
 * 
 * gcc -O0 -ggdb -Wall -o mysensor mysensor.c client.c -I/home/lprog/MyLinuxProg/LinuxHomework/Client
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>

#include "client.h"

/* Declare global variables */
uint8_t exitCondition;
char measDataFilePath[MEAS_DATA_FILE_PATH_LEN];

int main(int argc, char* argv[]) {
    
    char trash[256];
    struct pollfd pollArray[POLL_ARRAY_SIZE];
    
    /* Check arguments */
    if((argc != 3) && (argc != 1)) {
        
        fprintf(stdout, "Usage: %s [server IP] [server port]\n", argv[0]);
        fflush(stdout);
        return EXIT_FAILURE;
    }
    
    /* Initialize global variables */
    exitCondition = COND_EXIT_FALSE;
    
    memset(measDataFilePath, 0 ,sizeof(measDataFilePath));
    initMeasDataFilePath(measDataFilePath);
    
    /* Initialize poll array */
    pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
    pollArray[POLL_ARRAY_SOCKET].events = POLLIN;
    pollArray[POLL_ARRAY_STDIN].fd = STDIN_FILENO;
    pollArray[POLL_ARRAY_STDIN].events = POLLIN;
    
    /* Connect to server if specified in the launch arguments */
    if(3 == argc) {
        
        connectServer(argv[1], argv[2], pollArray);
    }
    
    while(!exitCondition) {
        
        /* Polling... */
        if(poll(pollArray, POLL_ARRAY_SIZE, -1) > 0) {
            
            if(pollArray[POLL_ARRAY_SOCKET].revents & (POLLERR | POLLHUP)) {
                
                /* Server closed connection */
                
                fprintf(stdout, MSG_USER_WARN_SERVER_CLOSED);
                fprintf(stdout, MSG_USER_INFO_DISCONNECT);
                fflush(stdout);
                
                close(pollArray[POLL_ARRAY_SOCKET].fd);
                pollArray[POLL_ARRAY_SOCKET].fd = INVALID_FD;
            }
            else if(pollArray[POLL_ARRAY_SOCKET].revents & (POLLIN)) {
                
                /* Message received from server */
                
                /*
                 * Default: server should not send data if not requested !
                 * Empty buffer to prevent possible errors.
                 */
                recv(pollArray[POLL_ARRAY_SOCKET].fd ,&trash, sizeof(trash), 0);
            }
            
            if(pollArray[POLL_ARRAY_STDIN].revents & (POLLIN)) {
                
                /* Message received from standard input */
                
                interpretUserCommand(pollArray);
            }
            
        }
    }
    
    /* Exit program */
    fprintf(stdout, MSG_USER_INFO_EXIT);
    fflush(stdout);
    
    return EXIT_SUCCESS;
}
