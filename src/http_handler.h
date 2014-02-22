#include <stdint.h>
#include <stdbool.h>

#define DEVICE_NAME_SIZE            15
#define RESOURCE_NAME_SIZE          20

#define RECEIVE_BUFFER_SIZE         1024
#define RECEIVE_BUFFER_LAST_INDEX   (RECEIVE_BUFFER_SIZE - 1)

#define TRANSMIT_BUFFER_SIZE        1024

#define MAX_RESOURCES               4   /* Per control */
#define MAX_METHODS                 4   /* Per resource */

#define HTTP_EOL_STR                "\r\n"
#define HTTP_EOL_LEN                2

#define HTTP_VERSION_LEN            8   /* "HTTP/X.X" */
                                    /* "GET / " VERSION \r\n */
#define HTTP_START_LINE_MIN_LEN     (6 + HTTP_VERSION_LEN) 
#define HTTP_SPACE_LEN              1   /* ' ' */

#define ASCII_DIGIT_OFFSET          48

#define HTTP_RESOURCE_START_CHAR    '/'

#define PRINT_MESSAGES              FALSE

#if PRINT_MESSAGES == TRUE  
    #define PRINT(STR, ...) \
        printf(STR, __VA_ARGS__)
    
    #define PRINT_LINE(STR) \
        printf("(%s %s:%d) " STR "\r\n", __FUNCTION__, __FILE__, __LINE__)
    
    #define PRINT_LINE_ARGS(STR, ...) \
        printf("(%s %s:%d): " STR "\r\n", __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
    #define PRINT(...) 
    #define PRINT_LINE(...) 
    #define PRINT_LINE_ARGS(...) 
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
    methodType type;
    buf resource;
    httpVersion version;


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
    char name[RESOURCE_NAME_SIZE];
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
    char deviceName[DEVICE_NAME_SIZE]; 
    resourceInformation resources[MAX_RESOURCES];
    parsingInformation parsed;
} controlInformation;

const char * parseHttp(httpParser * par, const char * data, const uint16_t size,
               void * user);

void registerControl(controlInformation * control);



