/*******************************************************************************
* Copyright (c) 2014, Alan Barr
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef __CLARITY_API__
#define __CLARITY_API__

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "ch.h"

#define CLARITY_MAX_RESOURCES               4   /* Per control */
#define CLARITY_MAX_METHODS                 4   /* Per resource */

#define CLARITY_MAX_HEADERS                 10  /* Maximum stored headers */

#define CLARITY_PRINT_MESSAGES              TRUE

/* TODO should be here? */
#define CLARITY_MAX_CONTENT_LENGTH_DIGITS   6   /* Max number of characters in string + NULL */

#define CLARITY_MAX_URL_LENGTH              50  /* URL lengths */

#define CLARITY_MAX_POWER_ADDRESSES         3

#define CLARITY_MAX_AP_STR_LEN              16

#define CLARITY_MAX_CONTENT_LENGTH_DIGITS   6   /* Max number of characters in string + NULL */

#define CLARITY_ERRORS                              \
    CLARITY_ERROR(CLARITY_SUCCESS)                  \
    CLARITY_ERROR(CLARITY_ERROR_UNDEFINED)          \
    CLARITY_ERROR(CLARITY_ERROR_REMOTE_RESPONSE)    \
    CLARITY_ERROR(CLARITY_ERROR_CC3000_WLAN)        \
    CLARITY_ERROR(CLARITY_ERROR_CC3000_SOCKET)      \
    CLARITY_ERROR(CLARITY_ERROR_CC3000_NETAPP)      \
    CLARITY_ERROR(CLARITY_ERROR_BUFFER_SIZE)        \
    CLARITY_ERROR(CLARITY_ERROR_RANGE)              \
    CLARITY_ERROR(CLARITY_ERROR_PARAMETER)          \
    CLARITY_ERROR(CLARITY_ERROR_STATE)

#ifdef CLARITY_ERROR
#undef CLARITY_ERROR
#endif
#define CLARITY_ERROR(E) E,
typedef enum {
    CLARITY_ERRORS
    CLARITY_MAX_ERROR
} clarityError;

#define CLARITY_HTTP_METHODS     \
    CLARITY_HTTP_METHOD(GET)     \
    CLARITY_HTTP_METHOD(HEAD)    \
    CLARITY_HTTP_METHOD(PUT)     \
    CLARITY_HTTP_METHOD(POST)    \
    CLARITY_HTTP_METHOD(TRACE)   \
    CLARITY_HTTP_METHOD(OPTIONS) \
    CLARITY_HTTP_METHOD(DELETE)  

#ifdef CLARITY_HTTP_METHOD
    #undef CLARITY_HTTP_METHOD
#endif 
#define CLARITY_HTTP_METHOD(M)    M,
typedef enum {
    CLARITY_HTTP_METHODS
    CLARITY_HTTP_METHOD_MAX
} clarityHttpMethodType;

typedef struct {
    const char * data; 
    uint16_t size;
} clarityBuf;

typedef struct {
    uint8_t major;
    uint8_t minor;
} clarityHttpVersion;

typedef struct {
    clarityBuf field;
    clarityBuf value;
} clarityHttpHeader;

typedef struct {
    clarityHttpMethodType type;
    clarityBuf resource;
    clarityHttpVersion version;
    clarityHttpHeader headers[CLARITY_MAX_HEADERS];
    clarityBuf content;
    clarityBuf body;
} clarityHttpRequestInformation;

typedef struct {
    clarityHttpVersion version;
    uint16_t code;
    /*TODO pointer to headers */
    /*TODO pointer to body */
} clarityHttpResponseInformation;

typedef struct {
    int32_t socket;
    void * user;
} clarityConnectionInformation; 

typedef uint32_t (*clarityHttpMethodCallback)(const clarityHttpRequestInformation * info, 
                                              clarityConnectionInformation * user);

typedef struct {
    clarityHttpMethodType type;
    clarityHttpMethodCallback callback;
} clarityHttpMethodInformation;

typedef struct {
    const char * name;
    clarityHttpMethodInformation methods[CLARITY_MAX_METHODS];
} clarityResourceInformation;

typedef struct { 
    clarityResourceInformation resources[CLARITY_MAX_RESOURCES];
} clarityHttpServerInformation; /*TODO Http Server */

typedef union {
    uint32_t ip;
    char url[CLARITY_MAX_URL_LENGTH]; /* XXX needs NULL of strnlen*/
} clarityAddressUrlIp;

typedef enum {
    CLARITY_ADDRESS_NONE,
    CLARITY_ADDRESS_URL,
    CLARITY_ADDRESS_IP
} clarityAddressType;

typedef struct {
    clarityAddressType type;
    clarityAddressUrlIp addr;
} clarityAddressInformation;

typedef enum {
    CLARITY_TRANSPORT_NONE,
    CLARITY_TRANSPORT_TCP,
    CLARITY_TRANSPORT_UDP
} clarityTransportType;

typedef struct {
    clarityTransportType type;
    clarityAddressInformation addr;
    uint16_t port;
} clarityTransportInformation;

#if 0
typedef struct {
    transportInformation tcp[MAX_POWER_ADDRESSES];
} powerStateNotification;
#endif

typedef struct {
    bool isStatic;
    uint32_t ip;
    uint32_t subnet;
    uint32_t gateway;
    uint32_t dns;
} clarityDeviceIpInformation;


typedef struct {
    /* TODO smart config profiles etc */
#if 0
    enum connMeth;
#endif
    char name[CLARITY_MAX_AP_STR_LEN];
    uint8_t secType;
    char password[CLARITY_MAX_AP_STR_LEN];
    clarityDeviceIpInformation deviceIp;
} clarityAccessPointInformation;



/* Initialisation */
typedef void (*clarityUnresponsiveCallback)(void);

typedef void (*clarityPrintCb)(const char * fmt, ...);

clarityError clarityInit(Mutex * cc3000ApiMtx,
                         clarityUnresponsiveCallback unrespCb,
                         clarityAccessPointInformation * accessPointConnection,
                         clarityPrintCb printCb);

clarityError clarityShutdown(void);

/* Mgmt */
void clarityCC3000ApiLock(void);
void clarityCC3000ApiUnlock(void);
clarityError clarityRegisterProcessStarted(void);
clarityError clarityRegisterProcessFinished(void);

/* HTTP Server */
clarityError clarityHttpServerStart(clarityHttpServerInformation * control);
clarityError clarityHttpServerStop(void);
clarityError clarityHttpServerSendInCb(const clarityConnectionInformation * conn,
                                       const void * data, uint16_t length);
clarityError clarityHttpBuildResponseTextPlain(char * clarityBuf,
                                               uint16_t clarityBufSize,
                                               uint8_t code,
                                               const char * message,
                                               const char * bodyString);
/* HTTP Client */
typedef struct {
    bool closeOnComplete;   /* Close persistant connection after response obtained */
    bool pipeline;
    /* Private */
    bool connected;         /* connection established */  
    int32_t socket;         /* open socket */
    uint32_t requestsPipelined; /* number of pipelined messages */
} clarityHttpConfiguration;

clarityError clarityHttpSendRequest(clarityTransportInformation * transport,
                                    clarityHttpConfiguration * config,
                                    char * buf,
                                    uint16_t bufSize,
                                    uint16_t requestSize,
                                    clarityHttpResponseInformation * response);

/* manually call for requests send pipleline=true */
clarityError clarityHttpPipelinedResponse(clarityHttpConfiguration * config,
                                          char * buf,
                                          uint16_t bufSize,
                                          clarityHttpResponseInformation * response);

clarityError clarityHttpBuildPost(char * buf, uint16_t bufSize,
                                  const char * device,
                                  const char * resource,
                                  const char * content, 
                                  clarityHttpConfiguration * persistant);

/* SNTP Client */
typedef struct {
    uint8_t hour;   /* 0 - 24 */
    uint8_t minute; /* 0 - 59 */
    uint8_t second; /* 0 - 59 */
} clarityTime;

typedef struct {
    uint8_t year;   /* 0 - 99 (tens)*/
    uint8_t month;  /* 1 (Ja) - 12 (Dec) */
    uint8_t date;   /* 1 - 31 */
    uint8_t day;    /* 1 (Mon) - 7 (Sun)*/
} clarityDate;

typedef struct {
    clarityTime time;
    clarityDate date;
} clarityTimeDate;

clarityError clarityGetSntpTime(char * buf, uint16_t bufSize,
                                uint32_t * ntpSeconds);

/* Time Conversions */
#include <time.h>
clarityError clarityTimeFromUnix(clarityTimeDate * clarTD, time_t unix);
clarityError clarityTimeToUnix(clarityTimeDate * clarTD, time_t * unix);
clarityError clarityTimeFromSntp(clarityTimeDate * clarTD, uint32_t sntp);
clarityError clarityTimeIncrement(clarityTimeDate * clarTD, uint32_t seconds);



#endif /* __CLARITY_API__ */


