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

#define MAX_RESOURCES               4   /* Per control */
#define MAX_METHODS                 4   /* Per resource */

#define MAX_HEADERS                 10  /* Maximum stored headers */

/* TODO should be here? */
#define MAX_CONTENT_LENGTH_DIGITS   6   /* Max number of characters in string + NULL */

#define MAX_URL_LENGTH              50  /* URL lengths */

#define MAX_POWER_ADDRESSES         3

#define MAX_AP_STR_LEN              16

#define MAX_CONTENT_LENGTH_DIGITS   6   /* Max number of characters in string + NULL */

#define METHODS     \
    METHOD(GET)     \
    METHOD(HEAD)    \
    METHOD(PUT)     \
    METHOD(POST)    \
    METHOD(TRACE)   \
    METHOD(OPTIONS) \
    METHOD(DELETE)  

#ifdef METHOD
    #undef METHOD
#endif 
#define METHOD(M)    M,

# if 0
#define METHOD_TO_MASK(M) (0x1<<M)
#endif 

typedef enum {
    METHODS
    METHOD_TYPE_MAX
} methodType;

#if 0
#ifdef METHOD
    #undef METHOD
#endif 
#define METHOD(M)    M ## _MASK = METHOD_TO_MASK(M),

typedef enum {
    METHODS
    METHODS_MASK_MAX
} methodMask;
#endif

#define CLARITY_ERRORS                              \
    CLARITY_ERROR(CLARITY_SUCCESS)                  \
    CLARITY_ERROR(CLARITY_ERROR)                    \
    CLARITY_ERROR(CLARITY_ERROR_UNDEFINED)          \
    CLARITY_ERROR(CLARITY_ERROR_RESPONSE)           \
    CLARITY_ERROR(CLARITY_ERROR_CC3000_WLAN)        \
    CLARITY_ERROR(CLARITY_ERROR_CC3000_SOCKET)      

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
} buf;

typedef struct {
    uint8_t major;
    uint8_t minor;
} httpVersion;

typedef struct {
    buf field;
    buf value;
} header;

typedef struct {
    methodType type;
    buf resource;
    httpVersion version;
    header headers[MAX_HEADERS];
    buf content;
    buf body;
} httpInformation; /* rename */

typedef struct {
    httpVersion version;
    uint16_t code;
} httpResInformation;

typedef struct {
    int32_t socket;
    void * user;
} connectionInformation; 

typedef uint32_t (*methodCallback)(const httpInformation * info, 
                                   connectionInformation * user);

typedef struct {
    methodType type;
    methodCallback callback;
} methodInformation;

typedef struct {
    const char * name;
#if 0
    methodMask methodsMask; /* Mask of all supported methods */ 
#endif
    methodInformation methods[MAX_METHODS];
} resourceInformation;

typedef struct { 
    const char * deviceName;
    resourceInformation resources[MAX_RESOURCES];
} controlInformation;

typedef union {
    uint32_t ip;
    char url[MAX_URL_LENGTH]; /* XXX needs NULL of strnlen*/
} addressUrlIp;

typedef enum {
    ADDRESS_NONE,
    ADDRESS_URL,
    ADDRESS_IP
} addressType;

typedef struct {
    addressType type;
    addressUrlIp addr;
} addressInformation;

typedef enum {
    TRANSPORT_NONE,
    TRANSPORT_TCP,
    TRANSPORT_UDP
} transportType;

typedef struct {
    transportType type;
    addressInformation addr;
    uint16_t port;
} transportInformation;

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
    char name[MAX_AP_STR_LEN];
    uint8_t secType;
    char password[MAX_AP_STR_LEN];
} accessPointInformation;


int32_t clarityInit(accessPointInformation * accessPointConnection);

/* HTTP Reply*/
int32_t claritySendInCb(const connectionInformation * conn,
                             const void * data, uint16_t length);
int32_t clarityHttpBuildResponseTextPlain(char * buf,
                                     uint16_t bufSize,
                                     uint8_t code,
                                     const char * message,
                                     const char * bodyString);

/* HTTP Server */
int32_t clarityHttpServerStart(Mutex * cc3000ApiMtx,
                                controlInformation * control);
int32_t clarityHttpServerKill(void);

/* CC3000 API Mutex Protection */
void clarityCC3000ApiLck(void);
void clarityCC3000ApiUnlck(void);


#define PRINT_MESSAGES              false

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


