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

/* TODO should be here? */
#define CLARITY_MAX_CONTENT_LENGTH_DIGITS   6   /* Max number of characters in string + NULL */

#define CLARITY_MAX_URL_LENGTH              50  /* URL lengths */

#define CLARITY_MAX_POWER_ADDRESSES         3

#define CLARITY_MAX_AP_STR_LEN              16

#define CLARITY_MAX_CONTENT_LENGTH_DIGITS   6   /* Max number of characters in string + NULL */

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

#define CLARITY_ERRORS                              \
    CLARITY_ERROR(CLARITY_SUCCESS)                  \
    CLARITY_ERROR(CLARITY_ERROR)                    \
    CLARITY_ERROR(CLARITY_ERROR_UNDEFINED)          \
    CLARITY_ERROR(CLARITY_ERROR_REMOTE_RESPONSE)    \
    CLARITY_ERROR(CLARITY_ERROR_CC3000_WLAN)        \
    CLARITY_ERROR(CLARITY_ERROR_CC3000_SOCKET)      \
    CLARITY_ERROR(CLARITY_ERROR_BUFFER_SIZE)

#ifdef CLARITY_ERROR
#undef CLARITY_ERROR
#endif
#define CLARITY_ERROR(E) E,
typedef enum {
    CLARITY_ERRORS
    CLARITY_MAX_ERROR
} clarityError;

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
} clarityHttpInformation; /* rename */

typedef struct {
    clarityHttpVersion version;
    uint16_t code;
} clarityHttpResInformation; /*TODO  Response */

typedef struct {
    int32_t socket;
    void * user;
} clarityConnectionInformation; 

typedef uint32_t (*clarityHttpMethodCallback)(const clarityHttpInformation * info, 
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
# if 0
    const char * deviceName;
#endif
    clarityResourceInformation resources[CLARITY_MAX_RESOURCES];
} clarityControlInformation; /*TODO Http Server */

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
    /* TODO smart config profies etc */
#if 0
    enum connMeth;
#endif
    char name[CLARITY_MAX_AP_STR_LEN];
    uint8_t secType;
    char password[CLARITY_MAX_AP_STR_LEN];
} clarityAccessPointInformation;


int32_t clarityInit(clarityAccessPointInformation * accessPointConnection);

/* HTTP Reply*/
int32_t claritySendInCb(const clarityConnectionInformation * conn,
                             const void * data, uint16_t length);
int32_t clarityHttpBuildResponseTextPlain(char * clarityBuf,
                                     uint16_t clarityBufSize,
                                     uint8_t code,
                                     const char * message,
                                     const char * bodyString);

/* HTTP Server */
clarityError clarityHttpServerStart(Mutex * cc3000ApiMtx,
                                    clarityControlInformation * control);
clarityError clarityHttpServerKill(void);

/* CC3000 API Mutex Protection */
void clarityCC3000ApiLck(void);
void clarityCC3000ApiUnlck(void);


#define PRINT_MESSAGES              true

#if defined(CLAR_PRINT_MESSAGES) && CLAR_PRINT_MESSAGES == true  

    #define CLAR_PRINT_ERROR() \
        printf("(%s:%d) ERROR.\r\n", __FILE__, __LINE__)

    #define CLAR_PRINT(STR, ...) \
        printf(STR, __VA_ARGS__)
    
    #define CLAR_PRINT_LINE(STR) \
        printf("(%s:%d) " STR "\r\n", __FILE__, __LINE__)
    
    #define CLAR_PRINT_LINE_ARGS(STR, ...) \
        printf("(%s:%d): " STR "\r\n", __FILE__, __LINE__, __VA_ARGS__)
#else
    #define CLAR_PRINT_ERROR()
    #define CLAR_PRINT(...) 
    #define CLAR_PRINT_LINE(STR) 
    #define CLAR_PRINT_LINE_ARGS(...) 
#endif


#endif /* __CLARITY_API__ */


