#include <stdint.h>
#include <stdbool.h>

#define DEVICE_NAME_SIZE            15
#define RESOURCE_NAME_SIZE          20

#define RECEIVE_BUFFER_SIZE         1024
#define RECEIVE_BUFFER_LAST_INDEX   (RECEIVE_BUFFER_SIZE - 1)

#define TRANSMIT_BUFFER_SIZE        1024

#if 0
#define MAX_CONTROL                 1
#endif
#define MAX_RESOURCES               4   /* Per control */
#define MAX_METHODS                 4   /* Per resource */


#define PRINT(STR, ...) \
    printf(STR, __VA_ARGS__)

#define PRINT_LINE(STR,...) \
    printf("%s: " STR "\r\n", __FUNCTION__, __VA_ARGS__)

typedef struct {
    char buffer[RECEIVE_BUFFER_SIZE];
    uint16_t start;
    uint16_t end;
    uint16_t taken;
} readBuffer;


typedef enum {
    NONE   = 0x00,
    GET    = 0x01,
    PUT    = 0x02,
    DELETE = 0x04,
    POST   = 0x08,
    HEAD   = 0x10,
} methodType;


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
    bool completed;
} parsingInformation;


typedef struct { 
    char deviceName[DEVICE_NAME_SIZE]; 
    resourceInformation resources[MAX_RESOURCES];
    
    parsingInformation parsed;
} controlInformation;

void parseHttp(const char * data, const uint16_t size, void * user);
void registerControl(controlInformation * control);



