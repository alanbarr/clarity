#include "http_handler.h"
#include "http_parser.h"

#include <stdio.h> /* TODO */
#include <string.h>

static http_parser parser;
static http_parser_settings settings;
static controlInformation * control = NULL; 

/* Returns number of bytes that can be stored to the buffer in one 
 * continous copy */
uint16_t bufferAvailable(readBuffer * buffer)
{
    if (buffer->taken == RECEIVE_BUFFER_SIZE)
    {
        return 0;
    }

    else if (buffer->end == RECEIVE_BUFFER_LAST_INDEX)
    {
        return buffer->start;
    }

    else 
    {
        return RECEIVE_BUFFER_LAST_INDEX - buffer->end;
    }
}

/* Pointer to free buffer space.
 * Only use if bufferAvailable > 1 */
char * receiveBufferWriteTo(readBuffer * buffer) 
{ 
    return (buffer->end == RECEIVE_BUFFER_LAST_INDEX ? 
                                        &(buffer->buffer[0]) : 
                                        &(buffer->buffer[buffer->end + 1]));
}

/* Updates buffer after a write.
 * Update imediately after using receiveBufferWriteTo() */
void receiveBufferWriteUpdate(readBuffer * buffer, uint16_t size) 
{ 
    if (buffer->end == RECEIVE_BUFFER_LAST_INDEX)
    {
        buffer->end = 0;
        size -= 1;
    }
    buffer->end += size;
}


static int onUrl(http_parser* par, const char *data, size_t length)
{
    uint8_t resourceIndex;
    uint8_t methodIndex;
    int index;
    methodType method = NONE;

    PRINT_LINE("Alan Debug %s", __FUNCTION__);

    control->parsed.resource = NULL;
    control->parsed.method = NULL;

    for (index = 0; index <length; index++)
    {
        PRINT("%c", data[index]);
    }

    if (length > RESOURCE_NAME_SIZE)
    {
        return -1;
    }

    for (resourceIndex = 0; resourceIndex < MAX_RESOURCES; resourceIndex++)
    {
        if (strncmp(data, control->resources[resourceIndex].name,
                    length) == 0)
        {
            control->parsed.resource = &control->resources[resourceIndex];
            switch (par->method)
            {   
                case HTTP_GET:
                    method = GET;
                    break;
                case HTTP_PUT:
                    method = PUT;   
                    break;
                case HTTP_POST:
                    method = POST;
                    break;
                case HTTP_DELETE:
                    method = DELETE;
                    break;
                case HTTP_HEAD:
                    method = HEAD;
                    break;
                default:
                    method = NONE;
                    break;
            }
            break;
        }
    }

    if (method != NONE &&
        (control->parsed.resource->methodsMask & method) != 0)
    {
       for(methodIndex = 0; methodIndex < MAX_METHODS; methodIndex++)
       {
           if (control->parsed.resource->methods[methodIndex].type == method)
           {
               control->parsed.method = &control->parsed.resource->methods[methodIndex];
               PRINT_LINE("Got Method :%d", method);
               break;
           }
       }
    }
    else
    {

        PRINT_LINE("Didn't get method:%d", method);
        return -1;
    }

    return 0;
}   


static int onStatus(http_parser* par, const char *data, size_t length)
{
    int index = 0;
    PRINT("\n\nAlan Debug %s\n", __FUNCTION__);

    for (index = 0; index <length; index++)
    {
        PRINT("%c", data[index]);
    }

    return 0;
}   


static int onHeaderField(http_parser* par, const char *data, size_t length)
{
    int index = 0;
    PRINT("\n\nAlan Debug %s\n", __FUNCTION__);

    for (index = 0; index <length; index++)
    {
        PRINT("%c", data[index]);
    }

    return 0;
}   


static int onHeaderValue(http_parser* par, const char *data, size_t length)
{
    int index = 0;
    PRINT("\n\nAlan Debug %s\n", __FUNCTION__);

    for (index = 0; index <length; index++)
    {
        PRINT("%c", data[index]);
    }

    return 0;
}   


static int onBody(http_parser* par, const char *data, size_t length)
{
    int index = 0;
    PRINT("\n\nAlan Debug %s\n", __FUNCTION__);

    for (index = 0; index <length; index++)
    {
        PRINT("%c", data[index]);
    }

    return 0;
}   


static int onHeadersComplete(http_parser* par)
{
    PRINT_LINE("Alan Debug %s\n", __FUNCTION__);

    return 0;
}   


static int onComplete(http_parser*par)
{
    PRINT_LINE("In function: %s.", __FUNCTION__);
    control->parsed.completed = true;
    return 0;
}   


static int onMessageBegin(http_parser*par)
{
    memset(&control->parsed,0, sizeof (control->parsed));
    return 0;
}   
 

void registerControl(controlInformation * userControl)
{
    control = userControl;
}


void parseHttp(const char * data, const uint16_t size, void * user)
{
    size_t nparsed;

    settings.on_message_begin = onMessageBegin;
    settings.on_status = onStatus;
    settings.on_header_field = onHeaderField;
    settings.on_header_value = onHeaderValue;
    settings.on_headers_complete = onHeadersComplete;
    settings.on_body = onBody;
    settings.on_url = onUrl;
    settings.on_message_complete = onComplete;

    http_parser_init(&parser, HTTP_REQUEST);
    nparsed = http_parser_execute(&parser, &settings, data, size);
    PRINT("\n\nnparsed: %u\n", (unsigned int)nparsed);

    if (control->parsed.completed)
    {
        control->parsed.method->callback(NULL, NULL, 0, user);
    }
}



