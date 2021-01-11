/*
 * FileName:    client.h
 * Author:      Adam Csizy
 * Neptun Code: ******
 * 
 * Desc.:       Header file containing macros, global variable and function
 *              declarations shared across the application source files.
 * 
 *              Note that some macros marked as 'SHARED' must be synchronized with
 *              the corresponding items of server.h header file.
 */

#include <limits.h>
#include <poll.h>
#include <stdint.h>

/* Const message strings */
#define MSG_USER_INFO_CONF_FOOTER                   ("\n------------------------------------------\n")
#define MSG_USER_INFO_CONF_HEADER                   ("\n----- ACTUAL CONFIG OF BME280 SENSOR -----\n\n")
#define MSG_USER_INFO_CONF_FILTER                   ("IIR filter config:\t%s\n")
#define MSG_USER_INFO_CONF_HUMIDITY                 ("Humidity config:\t%s\n")
#define MSG_USER_INFO_CONF_PERIOD                   ("Period config:\t\t%d [sec]\n")
#define MSG_USER_INFO_CONF_PRESSURE                 ("Pressure config:\t%s\n")
#define MSG_USER_INFO_CONF_TEMPERATURE              ("Temperature config:\t%s\n")
#define MSG_USER_INFO_CONNECT_SUCCESS               ("[INFO] Connected to server.\n")
#define MSG_USER_INFO_DISCONNECT                    ("[INFO] Disconnected from server.\n")
#define MSG_USER_INFO_DISCONNECT_NONE               ("[INFO] There is no server to disconnect from.\n")
#define MSG_USER_INFO_EXIT                          ("[INFO] Exited program.\n")
#define MSG_USER_INFO_HINT_CONF                     ("[INFO] Hint: sconf <TMP|PRS|HUM|IIR|PRD> <ON|OFF> [value].\n")
#define MSG_USER_INFO_RECV_FDATA_SUCCESS            ("[INFO] Saved measurement data from remote server to local file.\n")
#define MSG_USER_INFO_REQ_SUCCESS                   ("[INFO] Client request completed.\n")
#define MSG_USER_REQ_NAME                           ("Username: ")
#define MSG_USER_REQ_PASSWORD                       ("Password: ")
#define MSG_USER_WARN_CONNECT_FAIL                  ("[WARNING] Failed to connect to server.\n")
#define MSG_USER_WARN_CONNECT_FAIL_ADDR             ("[WARNING] Cannot resolve IP adress.\n")
#define MSG_USER_WARN_CONNECT_FAIL_AUTH             ("[WARNING] Invalid username or password.\n")
#define MSG_USER_WARN_CONNECT_FAIL_EXIST            ("[WARNING] Connection to a server already exists.\n")
#define MSG_USER_WARN_CONNECT_FAIL_SOCK             ("[WARNING] Failed to open client socket.\n")
#define MSG_USER_WARN_DISCONNECT_FAIL               ("[WARNING] Failed to disconnect from server.\n")
#define MSG_USER_WARN_DISCONNECT_FAIL_REQ           ("[WARNING] Failed to send disconnect request to server.\n")
#define MSG_USER_WARN_DISCONNECT_FAIL_SOCK          ("[WARNING] Failed to close client socket.\n")
#define MSG_USER_WARN_INVALID_ARG                   ("[WARNING] Invalid argument.\n")
#define MSG_USER_WARN_INVALID_CMD                   ("[WARNING] Invalid command.\n")
#define MSG_USER_WARN_INVALID_CONF_PRD_ARG          ("[WARNING] Invalid config period argument.\n")
#define MSG_USER_WARN_INVALID_CONF_STAT_ARG         ("[WARNING] Invalid config status argument.\n")
#define MSG_USER_WARN_INVALID_CONF_TYPE_ARG         ("[WARNING] Invalid config type argument.\n")
#define MSG_USER_WARN_INVALID_CONF_VAL_ARG          ("[WARNIGN] Invalid config value argument.\n")
#define MSG_USER_WARN_MEAS_FILE_OPEN_FAIL           ("[WARNING] Failed to open local measurement data file.\n")
#define MSG_USER_WARN_MEAS_FILE_WRITE_FAIL          ("[WARNING] Failed to write into local measurement data file.\n")
#define MSG_USER_WARN_MEAS_PATH_INIT_FAIL           ("[WARNING] Failed to initialize measurement data file path.\n")
#define MSG_USER_WARN_MISSING_ARG                   ("[WARNING] Missing argument.\n")
#define MSG_USER_WARN_NO_CONN                       ("[WARNING] There is no active connection with a server.\n")
#define MSG_USER_WARN_REQ_FAIL                      ("[WARNING] Request failed.\n")
#define MSG_USER_WARN_REQ_FAIL_SERVER               ("[WARNING] Request failed due to server side error.\n")
#define MSG_USER_WARN_REQ_INVALID                   ("[WARNING] Invalid client request.\n")
#define MSG_USER_WARN_REQ_NO_PERM                   ("[WARNING] Client has no permission for the issued request.\n")
#define MSG_USER_WARN_REQ_NAME_FAIL                 ("[WARNING] Failed to read username.\n")
#define MSG_USER_WARN_REQ_PWD_FAIL                  ("[WARNING] Failed to read password.\n")
#define MSG_USER_WARN_RES_FAIL                      ("[WARNING] Failed to receive response from server.\n")
#define MSG_USER_WARN_RES_INVALID                   ("[WARNING] Invalid server response.\n")
#define MSG_USER_WARN_RECV_AUTH_FAIL                ("[WARNING] Failed to receive authentication response from server.\n")
#define MSG_USER_WARN_RECV_CONF_HUM_FAIL            ("[WARNING] Failed to receive humidity config from server.\n")
#define MSG_USER_WARN_RECV_CONF_IIR_FAIL            ("[WARNING] Failed to receive IIR filter config from server.\n")
#define MSG_USER_WARN_RECV_CONF_PRD_FAIL            ("[WARNING] Failed to receive measurement period from server.\n")
#define MSG_USER_WARN_RECV_CONF_PRS_FAIL            ("[WARNING] Failed to receive pressure config from server.\n")
#define MSG_USER_WARN_RECV_CONF_TMP_FAIL            ("[WARNING] Failed to receive temperature config from server.\n")
#define MSG_USER_WARN_RECV_FDATA_FAIL               ("[WARNING] Failed to receive measurement data from remote server.\n")
#define MSG_USER_WARN_RECV_FDATA_SIZE_FAIL          ("[WARNING] Failed to receive measurement data file size from server.\n")
#define MSG_USER_WARN_RECV_FDATA_NOT_AVL            ("[WARNING] Measurement data not available on server.\n")
#define MSG_USER_WARN_SEND_NAME_FAIL                ("[WARNING] Failed to send username to server.\n")
#define MSG_USER_WARN_SEND_PWD_FAIL                 ("[WARNING] Failed to send password to server.\n")
#define MSG_USER_WARN_SEND_REQ_FAIL                 ("[WARNING] Failed to send request to serer.\n")
#define MSG_USER_WARN_SERVER_CLOSED                 ("[WARNING] Server closed connection.\n")

/* Measurement data storage related macros */
#define MEAS_DATA_FD_INVALID                       (-1)                // Measurement data invalid file descriptor
#define MEAS_DATA_FILE_NAME                        ("meas_data")   // Measurement data file name
#define MEAS_DATA_FILE_NAME_LEN                    (14)                // Measurement data file name length
#define MEAS_DATA_FILE_PATH_LEN                    (PATH_MAX + 1 + MEAS_DATA_FILE_NAME_LEN)

/* Conditions */
#define COND_EXIT_FALSE                             ((uint8_t)0)
#define COND_EXIT_TRUE                              ((uint8_t)1)

/* Pollset entries */
#define POLL_ARRAY_SIZE                             (2)
#define POLL_ARRAY_SOCKET                           (0)
#define POLL_ARRAY_STDIN                            (1)

#define INVALID_FD                                  ((int)-1)

#define INPUT_BUFFER_SIZE                           (60)
#define MAX_NUM_OF_ARGS                             (4)

/* User command string representations */
#define STR_CMD_CONNECT                             ("conn")
#define STR_CMD_DISCONNECT                          ("dconn")
#define STR_CMD_SET_CONFIG                          ("sconf")
#define STR_CMD_SHOW_DATA                           ("show")
#define STR_CMD_GET_CONFIG                          ("gconf")
#define STR_CMD_REMOVE_DATA                         ("rmdat")
#define STR_CMD_GET_DATA                            ("gdat")
#define STR_CMD_EXIT                                ("exit")

#define STR_CMD_SCONF_IIR_FILTER                    ("IIR")
#define STR_CMD_SCONF_HUMIDITY                      ("HUM")
#define STR_CMD_SCONF_PERIOD                        ("PRD")
#define STR_CMD_SCONF_PRESSURE                      ("PRS")
#define STR_CMD_SCONF_TEMPERATURE                   ("TMP")

#define STR_CMD_SCONF_OFF                           ("OFF")
#define STR_CMD_SCONF_ON                            ("ON")

#define STR_CMD_SCONF_OVERSAMPLING_OFF              ("OS_OFF")      // [SHARED UNDER RES_STR_SCONF_OVERS...]
#define STR_CMD_SCONF_OVERSAMPLING_1X               ("OS_1X")       // [SHARED UNDER RES_STR_SCONF_OVERS...]
#define STR_CMD_SCONF_OVERSAMPLING_2X               ("OS_2X")       // [SHARED UNDER RES_STR_SCONF_OVERS...]
#define STR_CMD_SCONF_OVERSAMPLING_4X               ("OS_4X")       // [SHARED UNDER RES_STR_SCONF_OVERS...]
#define STR_CMD_SCONF_OVERSAMPLING_8X               ("OS_8X")       // [SHARED UNDER RES_STR_SCONF_OVERS...]
#define STR_CMD_SCONF_OVERSAMPLING_16X              ("OS_16X")      // [SHARED UNDER RES_STR_SCONF_OVERS...]

#define STR_CMD_SCONF_IIR_COEFF_OFF                 ("COEFF_OFF")   // [SHARED UNDER RES_STR_SCONF_IIR_CO...]
#define STR_CMD_SCONF_IIR_COEFF_2                   ("COEFF_2")     // [SHARED UNDER RES_STR_SCONF_IIR_CO...]
#define STR_CMD_SCONF_IIR_COEFF_4                   ("COEFF_4")     // [SHARED UNDER RES_STR_SCONF_IIR_CO...]
#define STR_CMD_SCONF_IIR_COEFF_8                   ("COEFF_8")     // [SHARED UNDER RES_STR_SCONF_IIR_CO...]
#define STR_CMD_SCONF_IIR_COEFF_16                  ("COEFF_16")    // [SHARED UNDER RES_STR_SCONF_IIR_CO...]

/* User data related macros */
#define USR_NAME_MAX_LENGTH                         (32)        // [SHARED]
#define USR_PWD_MAX_LENGTH                          (32)        // [SHARED]

/* Server response related macros */
#define RES_CODE_AUTH_FAIL                          (0x00)      // [SHARED] Client authentication failed
#define RES_CODE_AUTH_SUCCESS                       (0x01)      // [SHARED] Client authentication suceeded
#define RES_CODE_REQ_ACCEPT                         (0x02)      // [SHARED] Client request accepted
#define RES_CODE_REQ_FAIL                           (0x03)      // [SHARED] Client request failed (server side error)
#define RES_CODE_REQ_INVALID                        (0x04)      // [SHARED] Client request invalid
#define RES_CODE_REQ_NO_PERM                        (0x05)      // [SHARED] Client does not have permission for request
#define RES_CODE_REQ_SUCCESS                        (0x06)      // [SHARED] Client request succeeded

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

/* Global variable declarations */
extern uint8_t exitCondition;
extern char measDataFilePath[MEAS_DATA_FILE_PATH_LEN];

/* Function declarations */

/*
 * Function 'initMeasDataFilePath': initializes file path for file containing measurement data.
 */
int initMeasDataFilePath(char filePathBuf[]);

/*
 * Function 'connectServer': connects to the remote server.
 */
int connectServer(const char *ipAddress, const char *portNumber, struct pollfd pollArray[]);

/* 
 * Function 'disconnectServer': disconnects from the remote server.
 */
int disconnectServer(struct pollfd pollArray[]);

/*
 * Function 'exitProgram': implements exit preparations.
 */
int exitProgram(struct pollfd pollArray[]);

/*
 * Function 'setSensorConfig': requests server to set sensor configuration.
 */
int setSensorConfig(struct pollfd pollArray[], const char* args[]);

/*
 * Function 'getSensorConfig': requests server to get sensor configuration.
 */
int getSensorConfig(struct pollfd pollArray[]);

/*
 * Function 'getSensorData': requests server to get sensor data.
 */
int getSensorData(struct pollfd pollArray[]);

/*
 * Function 'removeSensorData': requests server to remove sensor data.
 */
int removeSensorData(struct pollfd pollArray[]);

/*
 * Function 'showSensorData': show measurement data records transferred from server.
 */
int showSensorData(struct pollfd pollArray[]);

/* 
 * Function 'interpretUserCommand': interprets the command line user isntructions.
 */
int interpretUserCommand(struct pollfd pollArray[]);
