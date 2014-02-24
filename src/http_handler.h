#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_RESOURCES               4   /* Per control */
#define MAX_METHODS                 4   /* Per resource */

#define MAX_HEADERS                 10  /* Maximum stored headers */

#define MAX_DEVICE_NAME_SIZE        15
#define MAX_RESOURCE_NAME_SIZE      20

#define MAX_CONTENT_LENGTH_DIGITS   6 /* Max number of characters in sring + NULL */

#define RECEIVE_BUFFER_SIZE         1024
#define RECEIVE_BUFFER_LAST_INDEX   (RECEIVE_BUFFER_SIZE - 1)

#define TRANSMIT_BUFFER_SIZE        1024


#define HTTP_EOL_STR                "\r\n"
#define HTTP_EOL_LEN                2

#define HTTP_CONTENT_TYPE_STR       "Content-Type: "
#define HTTP_CONTENT_TYPE_LEN       14

#define HTTP_CONTENT_LENGTH_STR     "Content-Length: " 
#define HTTP_CONTENT_LENGTH_LEN     16


#define HTTP_VERSION_LEN            8   /* "HTTP/X.X" */
                                    /* "GET / " VERSION \r\n */
#define HTTP_START_LINE_MIN_LEN     (6 + HTTP_VERSION_LEN) 
#define HTTP_SPACE_LEN              1   /* ' ' */

#define ASCII_DIGIT_OFFSET          48

#define HTTP_RESOURCE_START_CHAR    '/'

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

#define METHODS     \
    METHOD(GET)     \
    METHOD(HEAD)    \
    METHOD(PUT)     \
    METHOD(POST)    \
    METHOD(TRACE)   \
    METHOD(OPTIONS) \
    METHOD(DELETE)  \

#ifdef METHOD
    #undef METHOD
#endif 
#define METHOD(M)    M,

typedef enum {
    METHODS
    METHOD_INVALID
} methodType;

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
} httpParser;

typedef struct {
    char buffer[RECEIVE_BUFFER_SIZE];
    uint16_t start;
    uint16_t end;
    uint16_t taken;
} readBuffer;


typedef enum {
    HEADER_TYPE_NONE,
    CONTENT_TYPE
} headerType;


typedef uint32_t (*methodCallback)(const char * body, const char * data,
                                   uint16_t size, void * user);

typedef struct {
    methodType type;
    methodCallback callback;
} methodInformation;


typedef struct {
    char name[MAX_RESOURCE_NAME_SIZE];
    methodType methodsMask; /* Mask of all supported methods */
    methodInformation methods[MAX_METHODS];
} resourceInformation;


typedef struct {
    resourceInformation * resource;
    methodInformation * method;
    headerType nextHeaderValue;
    bool completed;
} parsingInformation;


typedef struct { 
    char deviceName[MAX_DEVICE_NAME_SIZE]; 
    resourceInformation resources[MAX_RESOURCES];
    parsingInformation parsed;
} controlInformation;

const char * parseHttp(httpParser * par, const char * data, const uint16_t size,
               void * user);

void registerControl(controlInformation * control);



