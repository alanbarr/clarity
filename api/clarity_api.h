#ifndef __CLARITY_API__
#define __CLARITY_API__

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_RESOURCES               4   /* Per control */
#define MAX_METHODS                 4   /* Per resource */

#define MAX_HEADERS                 10  /* Maximum stored headers */

#define MAX_CONTENT_LENGTH_DIGITS   6 /* Max number of characters in sring + NULL */

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

#define METHOD_TO_MASK(M) (0x1<<M)

typedef enum {
    METHODS
    METHOD_TYPE_MAX
} methodType;

#ifdef METHOD
    #undef METHOD
#endif 
#define METHOD(M)    M ## _MASK = METHOD_TO_MASK(M),

typedef enum {
    METHODS
    METHODS_MASK_MAX
} methodMask;

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
} httpInformation;

typedef enum {
    HEADER_TYPE_NONE,
    CONTENT_TYPE
} headerType;

typedef uint32_t (*methodCallback)(const httpInformation * info, void * user);

typedef struct {
    methodType type;
    methodCallback callback;
} methodInformation;

typedef struct {
    const char * name;
    methodMask methodsMask; /* Mask of all supported methods */ 
    methodInformation methods[MAX_METHODS];
} resourceInformation;

typedef struct { 
    const char * deviceName;
    resourceInformation resources[MAX_RESOURCES];
} controlInformation;

const char * httpParse(httpInformation * info,
                       const char * data, const uint16_t size);

void httpRegisterControl(controlInformation * control);
const char * clarityProcess(controlInformation * control, 
                            httpInformation * info, 
                            const char * data, uint16_t size, void * user);


#define PRINT_MESSAGES              false

#if defined(HTTP_PRINT_MESSAGES) && HTTP_PRINT_MESSAGES == true  
    #define HTTP_PRINT(STR, ...) \
        printf(STR, __VA_ARGS__)
    
    #define HTTP_PRINT_LINE(STR) \
        printf("(%s %s:%d) " STR "\r\n", __FUNCTION__, __FILE__, __LINE__)
    
    #define HTTP_PRINT_LINE_ARGS(STR, ...) \
        printf("(%s %s:%d): " STR "\r\n", __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
    #define HTTP_PRINT(...) 
    #define HTTP_PRINT_LINE(...) 
    #define HTTP_PRINT_LINE_ARGS(...) 
#endif


#endif /* __CLARITY_API__ */


