/* 
 * FileName:    myserver.h
 * Author:      Adam Csizy
 * Neptun Code: ******
 * 
 * Desc.:       Header file containing macros, type definitions, global variable and
 *              function declarations shared across the application source files.
 * 
 *              Note that some macros marked as 'SHARED' must be synchronized with
 *              the corresponding items of client.h header file.
 */

#include <limits.h>
#include <pthread.h>
#include <stdint.h>

/* Server config related macros */
#define SERVER_PORT_NUMBER                          (2233)
#define SERVER_PENDING_QUEUE_LIMIT                  (4)
#define SERVER_SYSLOG_NAME                          ("SensorServer")
#define SERVER_THREAD_POOL_SIZE                     (4)

/* Const log strings */
#define LOG_SYS_ERR_CLIENT_REQ_CONF_FAIL            ("Failed to receive client config request. (%s)\n")
#define LOG_SYS_ERR_CLIENT_REQ_CONF_TYP_INVAL       ("Invalid configuration type requested by client. (%s)\n")
#define LOG_SYS_ERR_CLIENT_REQ_CONF_VAL_INVAL       ("Invalid configuration value requested by client. (%s)\n")
#define LOG_SYS_ERR_CLIENT_REQ_FAIL                 ("Failed to accomplish client request. (%s --> %x)\n")
#define LOG_SYS_ERR_CLIENT_REQ_RECV_FAIL            ("Failed to receive client request.\n")
#define LOG_SYS_ERR_INVALID_ARGS                    ("Invalid function arguments.\n")
#define LOG_SYS_ERR_INVALID_SOCK                    ("Invalid socket descriptor.\n")
#define LOG_SYS_ERR_SENS_INIT_FAIL                  ("Failed to initialize BME280 sensor.\n")
#define LOG_SYS_ERR_SENS_MEAS_FAIL                  ("Failed to get BME280 sensor measurement data.\n")
#define LOG_SYS_ERR_SENS_STGS_SET_FAIL              ("Failed to set BME280 sensor settings.\n")
#define LOG_SYS_ERR_SERVER_CONF_HUM_SEND_FAIL       ("Failed to send humidity config to client. (Client: %s)\n")
#define LOG_SYS_ERR_SERVER_CONF_IIR_SEND_FAIL       ("Failed to send IIR filter config to client. (Client: %s)\n")
#define LOG_SYS_ERR_SERVER_CONF_PRD_SEND_FAIL       ("Failed to send measurement period to client. (Client: %s)\n")
#define LOG_SYS_ERR_SERVER_CONF_PRS_SEND_FAIL       ("Failed to send pressure config to client. (Client: %s)\n")
#define LOG_SYS_ERR_SERVER_CONF_TMP_SEND_FAIL       ("Failed to send temperature config to client. (Client: %s)\n")
#define LOG_SYS_ERR_SERVER_FCONT_ACCESS_FAIL        ("Failed to access measurement data file: closed or not existing. (Client: %s)\n")
#define LOG_SYS_ERR_SERVER_FCONT_SEND_FAIL          ("Failed to send measurement data file content to client. (Client: %s)\n")
#define LOG_SYS_ERR_SERVER_FSIZE_SEND_FAIL          ("Failed to send saved data file size to client. (Client: %s)\n")
#define LOG_SYS_ERR_SERVER_FSTAT_GET_FAIL           ("Failed to get measurement data file information. \n")
#define LOG_SYS_ERR_SERVER_RMV_DATA_FAIL            ("Failed to remove measurement data requested by client. (Client: %s)\n")
#define LOG_SYS_ERR_SERVER_RES_FAIL                 ("Failed to send server response to client. (%s <-- %x)\n")
#define LOG_SYS_ERR_SERVER_SAVE_CLOSE_FAIL          ("Failed to close saved data file.\n")
#define LOG_SYS_ERR_SERVER_SAVE_HUMID_FAIL          ("Failed to save humidity measurement data.\n")
#define LOG_SYS_ERR_SERVER_SAVE_OPEN_FAIL           ("Failed to open or create saved data file.\n")
#define LOG_SYS_ERR_SERVER_SAVE_PATH_INIT_FAIL      ("Failed to initialize saved data file path.\n")
#define LOG_SYS_ERR_SERVER_SAVE_PRESS_FAIL          ("Failed to save pressure measurement data.\n")
#define LOG_SYS_ERR_SERVER_SAVE_TEMP_FAIL           ("Failed to save temperature measurement data.\n")
#define LOG_SYS_ERR_SERVER_SOCK_ACCEPT_FAIL         ("Failed to accept client connection on thread %d.\n")
#define LOG_SYS_ERR_SERVER_SOCK_BIND_FAIL           ("Failed to bind server socket.\n")
#define LOG_SYS_ERR_SERVER_SOCK_CLOSE_FAIL          ("Failed to close server socket.\n")
#define LOG_SYS_ERR_SERVER_SOCK_CREAT_FAIL          ("Failed to create server socket.\n")
#define LOG_SYS_ERR_SERVER_SOCK_LISTEN_FAIL         ("Failed to set server socket to passive (listen).\n")
#define LOG_SYS_ERR_SERVER_SOCK_OPT_FAIL            ("Failed to set server socket option.\n")
#define LOG_SYS_ERR_THREAD_MEAS_CREAT_FAIL          ("Failed to create measure thread.\n")
#define LOG_SYS_ERR_THREAD_SERV_CREAT_FAIL          ("Failed to create service thread.\n")
#define LOG_SYS_INFO_CLIENT_AUTH_FAIL               ("Client authentication failed: invalid username or password. (Thread: %d)\n")
#define LOG_SYS_INFO_CLIENT_AUTH_SUCCESS            ("Client authentication succeeded. (Thread: %d Client: %s)\n")
#define LOG_SYS_INFO_CLIENT_AUTH_FAIL_NOTIF_FAIL    ("Failed to notify client of unsuccessful authentication. (Thread: %d)\n")
#define LOG_SYS_INFO_CLIENT_AUTH_SUCCESS_NOTIF_FAIL ("Failed to notify client of successful authentication. (Thread: %d)\n")
#define LOG_SYS_INFO_CLIENT_CONN                    ("Client assigned to thread %d. IP: %s PORT: %s\n")
#define LOG_SYS_INFO_CLIENT_REQ_DISCONN             ("Client requested to disconnect. (%s)\n")
#define LOG_SYS_INFO_CLIENT_REQ_GET_CONF            ("Client requested to get sensor configuration. (%s)\n")
#define LOG_SYS_INFO_CLIENT_REQ_GET_DATA            ("Client requested to get measurement data. (%s)\n")
#define LOG_SYS_INFO_CLIENT_REQ_GET_DATA_SUCCESS    ("Transferring measurement data to client succeeded. (Client: %s)\n")
#define LOG_SYS_INFO_CLIENT_REQ_RMV_DATA            ("Client requested to remove measurement data. (%s)\n")
#define LOG_SYS_INFO_CLIENT_REQ_RMV_DATA_SUCCESS    ("Removing measurement data requested by client succeeded. (Client: %s)\n")
#define LOG_SYS_INFO_CLIENT_REQ_SET_CONF            ("Client requested to set sensor configuration. (%s)\n")
#define LOG_SYS_INFO_CLIENT_REQ_GET_CONF_SUCCESS    ("Sensor configuration data requested by client successfully transmitted. (Client: %s)\n")
#define LOG_SYS_INFO_CLIENT_REQ_SET_CONF_SUCCESS    ("Sensor configuration requested by client succeeded. (Client: %s)\n")
#define LOG_SYS_INFO_CLIENT_REQ_NO_PERM             ("Client does not have permission for request. (%s --> %x)\n")
#define LOG_SYS_INFO_SENS_MEAS_SUCCESS              ("BME280 sensor measurement completed.\n")
#define LOG_SYS_INFO_SERVER_START                   ("Starting daemon server...\n")
#define LOG_SYS_INFO_THREAD_SERVICE_END             ("Client service on thread %d ended.\n")
#define LOG_SYS_WARN_CLIENT_DISCONN_UNEX            ("Client disconnected unexpectedly on thread %d.\n")
#define LOG_SYS_WARN_CLIENT_NAME_RESOLVE_FAIL       ("Failed to resolve client host name. (Thread: %d)\n")
#define LOG_SYS_WARN_SOCK_SET_OPT_FAIL              ("Failed to set socket option. (Thread: %d)\n")

/* Sensor and measurement related macros */
#define MEAS_PERIOD_INIT_SEC                        (15)                // Initial measurement period
#define SAVED_DATA_FD_INVALID                       (-1)                // Invalid saved data file descriptor
#define SAVED_DATA_FILE_NAME                        ("meas_data")   // Saved data file name
#define SAVED_DATA_FILE_NAME_LEN                    (14)                // Saved data file name length
#define SAVED_DATA_FILE_PATH_LEN                    (PATH_MAX + 1 + SAVED_DATA_FILE_NAME_LEN)
#define SAVED_DATA_FILE_PATH_PI                     ("/home/pi/meas_data")

/* Pollset entries */
#define POLL_ARRAY_SIZE                             (1)
#define POLL_ARRAY_SOCKET                           (0)

/* User data related macros */
#define USR_GRP_GUEST                               (0)
#define USR_GRP_CONF                                (1)
#define USR_GRP_ADMIN                               (2)         // NOT USED

#define USR_NAME_MAX_LENGTH                         (32)        // [SHARED]
#define USR_PWD_MAX_LENGTH                          (32)        // [SHARED]

#define USR_DATA_ARRAY_SIZE                         (3)

/* Client request related macros */
#define REQ_CODE_DCONN                              (0x00)      // [SHARED] Request disconnection
#define REQ_CODE_SCONF                              (0x01)      // [SHARED] Request to set sensor configuration
#define REQ_CODE_GCONF                              (0x02)      // [SHARED] Request to get sensor configuration
#define REQ_CODE_RMDAT                              (0x04)      // [SHARED] Request to remove sensor data
#define REQ_CODE_GDAT                               (0x08)      // [SHARED] Request to get sensor data

#define REQ_CONF_PRD                                (0x01)      // [SHARED] Request to config period
#define REQ_CONF_IIR                                (0x02)      // [SHARED] Request to config IIR filter
#define REQ_CONF_HUM                                (0x03)      // [SHARED] Request to config humidity
#define REQ_CONF_PRS                                (0x04)      // [SHARED] Request to config pressure
#define REQ_CONF_TMP                                (0x05)      // [SHARED] Request to config temperature

#define REQ_CONF_TYPE_MASK                          (0x07)      // [SHARED] Config type mask

#define REQ_CONF_OFF                                (0x00)      // [SHARED] Config status off
#define REQ_CONF_ON                                 (0x08)      // [SHARED] Config status on

#define REQ_CONF_STATUS_MASK                        (0x08)      // [SHARED] Config status mask

#define REQ_CONF_OS_OFF                             (0x00)      // [SHARED] Config oversampling off
#define REQ_CONF_OS_1X                              (0x10)      // [SHARED] Config 1X oversampling
#define REQ_CONF_OS_2X                              (0x20)      // [SHARED] Config 2X oversampling
#define REQ_CONF_OS_4X                              (0x30)      // [SHARED] Config 4X oversampling
#define REQ_CONF_OS_8X                              (0x40)      // [SHARED] Config 8X oversampling
#define REQ_CONF_OS_16X                             (0x50)      // [SHARED] Config 16X oversampling

#define REQ_CONF_COEFF_OFF                          (0x00)      // [SHARED] Config filter coefficient off
#define REQ_CONF_COEFF_2                            (0x10)      // [SHARED] Config filter coefficient 2
#define REQ_CONF_COEFF_4                            (0x20)      // [SHARED] Config filter coefficient 4
#define REQ_CONF_COEFF_8                            (0x30)      // [SHARED] Config filter coefficient 8
#define REQ_CONF_COEFF_16                           (0x40)      // [SHARED] Config filter coefficient 16

#define REQ_CONF_VALUE_MASK                         (0xF0)      // [SHARED] Config value mask

#define REQ_HANDLE_ARRAY_SIZE                       (4)

#define REQ_ARRAY_SIZE                              (5)

/* Server response related macros */
#define RES_CODE_AUTH_FAIL                          (0x00)      // [SHARED] Client authentication failed
#define RES_CODE_AUTH_SUCCESS                       (0x01)      // [SHARED] Client authentication suceeded
#define RES_CODE_REQ_ACCEPT                         (0x02)      // [SHARED] Client request accepted
#define RES_CODE_REQ_FAIL                           (0x03)      // [SHARED] Client request failed (server side error)
#define RES_CODE_REQ_INVALID                        (0x04)      // [SHARED] Client request invalid
#define RES_CODE_REQ_NO_PERM                        (0x05)      // [SHARED] Client does not have permission for request
#define RES_CODE_REQ_SUCCESS                        (0x06)      // [SHARED] Client request succeeded

#define RES_STR_SCONF_OVERSAMPLING_OFF              ("OS_OFF")  // [SHARED UNDER STR_CMD_SCONF_OVERS...]
#define RES_STR_SCONF_OVERSAMPLING_1X               ("OS_1X")   // [SHARED UNDER STR_CMD_SCONF_OVERS...]
#define RES_STR_SCONF_OVERSAMPLING_2X               ("OS_2X")   // [SHARED UNDER STR_CMD_SCONF_OVERS...]
#define RES_STR_SCONF_OVERSAMPLING_4X               ("OS_4X")   // [SHARED UNDER STR_CMD_SCONF_OVERS...]
#define RES_STR_SCONF_OVERSAMPLING_8X               ("OS_8X")   // [SHARED UNDER STR_CMD_SCONF_OVERS...]
#define RES_STR_SCONF_OVERSAMPLING_16X              ("OS_16X")  // [SHARED UNDER STR_CMD_SCONF_OVERS...]

#define RES_STR_SCONF_IIR_COEFF_OFF                 ("COEFF_OFF")   // [SHARED UNDER STR_CMD_SCONF_IIR_CO...]
#define RES_STR_SCONF_IIR_COEFF_2                   ("COEFF_2")     // [SHARED UNDER STR_CMD_SCONF_IIR_CO...]
#define RES_STR_SCONF_IIR_COEFF_4                   ("COEFF_4")     // [SHARED UNDER STR_CMD_SCONF_IIR_CO...]
#define RES_STR_SCONF_IIR_COEFF_8                   ("COEFF_8")     // [SHARED UNDER STR_CMD_SCONF_IIR_CO...]
#define RES_STR_SCONF_IIR_COEFF_16                  ("COEFF_16")    // [SHARED UNDER STR_CMD_SCONF_IIR_CO...]

#define RES_STR_SCONF_ERR                           ("ERROR")
#define RES_STR_SCONF_OFF                           ("X")

/* Conditions */
#define COND_SERVICE_STOP_FALSE                     ((uint8_t)0)
#define COND_SERVICE_STOP_TRUE                      ((uint8_t)1)

/* Type definitions */

/* Predeclaration of user data */
struct UserData; 

/* Pointer to a client request handler function */
typedef int (*reqHandPntr)(int, const struct UserData *user, void*);

/* User data */
struct UserData {
    
    char name[USR_NAME_MAX_LENGTH];
    char password[USR_PWD_MAX_LENGTH];
    int groupe;
};

/* Request groupe permissions */
struct Request {
    
    uint8_t requestCode;
    reqHandPntr requestHandler;
    int lowestGroupe;
};

/* Global variable declarations */

extern int serverSocket;
extern int savedDataFd;

extern char savedDataFilePath[SAVED_DATA_FILE_PATH_LEN];

extern pthread_mutex_t sensorMutex;
extern pthread_mutex_t serviceMutex;
extern pthread_mutex_t savedDataMutex;

extern const struct UserData userDataArray[USR_DATA_ARRAY_SIZE];
extern const struct Request requestArray[REQ_ARRAY_SIZE];

extern struct identifier sensorId;     // Sensor interface identifier
extern struct bme280_dev sensorDev;    // Sensor device and settings
extern uint8_t sensorSettingSel;       // Sensor setting selection
extern uint32_t minDelay;              // Minimal delay after requesting a measurement
extern int measPeriod;                 // Measurement period [sec]

/* Function declarations */

/*
 * Function 'serviceThreadFunction': serves client connections.
 */
void* serviceThreadFunction(void *arg);

/*
 * Function 'measureThreadFunction': conducts consecutive measurements.
 */
void* measureThreadFunction(void *arg);

/*
 * Function 'authClient': authenticates client before connetion.
 */
int authClient(const char *username, const char *password, const struct UserData* *userRef);

/*
 * Function 'initSavedDataFilePath': initializes file path for file containing saved data.
 */
int initSavedDataFilePath(char filePathBuf[]);

/*
 * Function 'clientHandler': interprets and forwards client requests.
 */
int clientHandler(int serviceSocket, const struct UserData *user, int *serviceStopCondition);

/*
 * Function 'disconnectHandler': handles disconnection requested by client.
 */
int disconnectHandler(int clientSocket, const struct UserData *user, void *customArg);

/*
 * Function 'setConfigHandler': sets new sensor configuration requested by client.
 */
int setConfigHandler(int clientSocket, const struct UserData *user, void *customArg);

/*
 * Function 'getConfigHandler': returns current sensor configuration requested by client.
 */
int getConfigHandler(int clientSocket, const struct UserData *user, void *customArg);

/*
 * Function 'removeDataHandler': removes measurement data requested by client.
 */
int removeDataHandler(int clientSocket, const struct UserData *user, void *customArg);

/*
 * Function 'getDataHandler': returns measurement data requested by client.
 */
int getDataHandler(int clientSocket, const struct UserData *user, void *customArg);
