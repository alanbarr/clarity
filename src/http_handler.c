#include "http_handler.h"
#if 0
#include "http_parser.h"
#endif

#include <string.h>
#include <strings.h>
#include <ctype.h>

#ifdef METHOD
    #undef METHOD
#endif 
#define METHOD(M)    #M,
static const char * methodStrings[] = {
    METHODS
    "INVALID"
};

#define SHORTER(X,Y) (X<Y ? X : Y)

/* Length of string offset from end using string start and length */
#define P_OFFSET_LEN(START, LENGTH, OFFSET)   ((START+LENGTH) - (OFFSET))

/* TODO macro for pointer length via subtraction. */
#define P_DIFF_LEN(BIGGER, SMALLER) (BIGGER - SMALLER + 1)

#if 0
static http_parser parser;
static http_parser_settings settings;
#endif
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


#if 0
static int onUrl(http_parser* par, const char *data, size_t length)
{
    uint8_t resourceIndex;
    uint8_t methodIndex;
    int index;
    methodType method = METHOD_TYPE_NONE;

    HTTP_PRINT_LINE_ARGS("Alan Debug %s", __FUNCTION__);

    control->parsed.resource = NULL;
    control->parsed.method = NULL;

    for (index = 0; index <length; index++)
    {
        HTTP_PRINT("%c", data[index]);
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
                    method = METHOD_TYPE_NONE;
                    break;
            }
            break;
        }
    }

    if (method != METHOD_TYPE_NONE &&
        (control->parsed.resource->methodsMask & method) != 0)
    {
       for(methodIndex = 0; methodIndex < MAX_METHODS; methodIndex++)
       {
           if (control->parsed.resource->methods[methodIndex].type == method)
           {
               control->parsed.method = &control->parsed.resource->methods[methodIndex];
               HTTP_PRINT_LINE_ARGS("Got Method :%d", method);
               break;
           }
       }
    }
    else
    {

        HTTP_PRINT_LINE_ARGS("Didn't get method:%d", method);
        return -1;
    }

    return 0;
}   


static int onStatus(http_parser* par, const char *data, size_t length)
{
    int index = 0;
    HTTP_PRINT("\n\nAlan Debug %s\n", __FUNCTION__);

    for (index = 0; index <length; index++)
    {
        HTTP_PRINT("%c", data[index]);
    }

    return 0;
}   


static int onHeaderField(http_parser* par, const char *data, size_t length)
{
    HTTP_PRINT_LINE("\n\n\nDEBUG");
    int i;
    for (i=0;i<length;i++)
    {
        HTTP_PRINT("%c", data[i]);
    }
    if (strncasecmp(data, "Content-type", length) == 0)
    {
        HTTP_PRINT_LINE("Got content type");
        control->parsed.nextHeaderValue = CONTENT_TYPE;
    }
    return 0;
}   


static int onHeaderValue(http_parser* par, const char *data, size_t length)
{
    if (control->parsed.nextHeaderValue == CONTENT_TYPE)
    {
        if (strncasecmp(data, "text/plain", length) != 0)
        {
            return -1;
        }
    }

    return 0;
}   


static int onBody(http_parser* par, const char *data, size_t length)
{
    int index = 0;
    HTTP_PRINT("\n\nAlan Debug %s\n", __FUNCTION__);

    for (index = 0; index <length; index++)
    {
        HTTP_PRINT("%c", data[index]);
    }

    return 0;
}   


static int onHeadersComplete(http_parser* par)
{
    HTTP_PRINT_LINE_ARGS("Alan Debug %s\n", __FUNCTION__);

    return 0;
}   


static int onComplete(http_parser*par)
{
    HTTP_PRINT_LINE_ARGS("In function: %s.", __FUNCTION__);
    control->parsed.completed = true;
    return 0;
}   


static int onMessageBegin(http_parser*par)
{
    memset(&control->parsed,0, sizeof (control->parsed));
    return 0;
}   

#endif
 

void registerControl(controlInformation * userControl)
{
    control = userControl;
}


/* Returns first end of line character (i.e. if CRLF returns pointer to CR)*/
static const char * getEndOfLine(const char * data, uint16_t size)
{
    uint16_t index = 0;

    /* N.B if already on CR need to still behave OK */
    while (index < size)
    {
        if (index + (HTTP_EOL_LEN-1) > size)
        {
            return NULL;
        }

        else if (strncmp(&data[index], HTTP_EOL_STR, HTTP_EOL_LEN) == 0)
        {
            return &data[index];
        }
        index++;
    }
    return NULL;
}


/* Returns pointer to first character on the next line. */
static const char * getNextLine(const char * data, uint16_t size)
{
    const char * dataRtnd = getEndOfLine(data,size);

    if (dataRtnd == NULL)
    {
        HTTP_PRINT_LINE("getEndOfLine failed.");
        return NULL;
    }

    if ((dataRtnd + HTTP_EOL_LEN) > (data + size - 1))
    {
        HTTP_PRINT_LINE("Not enough room for EOL.");
        return NULL;
    }
    
    HTTP_PRINT_LINE("Should be returning success.");
    return (dataRtnd += HTTP_EOL_LEN);
}


static const char * parseHttpStartLine(const char * data,
                                       uint16_t size,
                                       httpParser * par)
{
    const char * c = data;
    const char * resource = NULL;
    const char * version = NULL;
    methodType type = 0;
    const char * eol = getEndOfLine(data, size);

    if (eol == NULL)
    {
        HTTP_PRINT_LINE("Could not find eol.");
        return NULL;
    }

    if ((eol-c) < HTTP_START_LINE_MIN_LEN)
    {
        HTTP_PRINT_LINE_ARGS("eol: %p c: %p line length: %d", eol,c,(int)(eol-c));
        HTTP_PRINT_LINE("FAIL");
        return NULL;
    }
    
    for (type = 0; type < METHOD_INVALID; type++)
    {
        if (*c == *methodStrings[type])
        {
            if (type == POST || type == PUT)
            {
                if (*(c+1) == 'O' ||
                    *(c+1) == 'o')
                {
                    type = POST;
                }
                else
                {
                    type = PUT;
                }
            }
            break;
        }
    }

    if (type == METHOD_INVALID)
    {
        HTTP_PRINT_LINE("FAIL");
        return NULL;
    }

    else if (strncasecmp(c, methodStrings[type],
                         SHORTER(strlen(methodStrings[type]), (eol-c)) != 0))
    {
        HTTP_PRINT_LINE("FAIL");
        return NULL;
    }

    par->type = type;

    c += strlen(methodStrings[type]);
    
    if (*c != ' ')
    {
        HTTP_PRINT_LINE("Space not found after type.");
        return NULL;
    }
    
    /* c is currently on space before resource. Move to start of resource */
    c++;

    if (*c == HTTP_RESOURCE_START_CHAR)
    {   
        resource = c;
    }
    else 
    {
        HTTP_PRINT_LINE("Resource string did not start as expected.");
        return NULL;
    }

    /* Move over resource to version */
    while((c+1) < eol)
    {
        if (*c == ' ')
        {
            version = c = (c+1); /* H of HTTP*/
            break;
        }
        c++;
    }
    
    if (version == NULL)
    {
        HTTP_PRINT_LINE("Version was NULL.");
        return NULL;
    }
    
    /* Not sure if there can be space after the version. */
    if (eol - version >= HTTP_VERSION_LEN)
    {
        if (strncasecmp(version, "HTTP/", 5) != 0)  /* TODO */
        {
            HTTP_PRINT_LINE("HTTP/ not present.");
            return NULL;
        }

        c += 5;                       /* Skip HTTP/ */

        /* First Digit */
        if (isdigit(*c) == 0)
        {
            HTTP_PRINT_LINE("Version Major was not a digit.");
            return NULL;
        }
        else
        {
            (par->version.major = *c - 48);
        }

        /* Check for dp */
        c++;                /* You'll find no OOP here */
        if (*c != '.')
        {
            HTTP_PRINT_LINE("Version DP not as expected.");
            return NULL;
        }

        /* Second Digit */
        c++;
        if (isdigit(*c) == 0)
        {
            HTTP_PRINT_LINE("Version Minor not a digit.");
            return NULL;
        }
        else
        {
            (par->version.minor = *c - ASCII_DIGIT_OFFSET);
        }
    }

    /* Update Resource */
    par->resource.data = resource;
    par->resource.size = version - resource - HTTP_SPACE_LEN;

    /* Success */
    return eol; /* todo range */
}


/* data is pointer to first byte on a new line */
static const char * parseHttpHeaders(const char * data,
                                     uint16_t size,
                                     httpParser * par)
{
    const char * const dataStart = data;
    header header;
    const char * lineStart = data;
    const char * eol = getEndOfLine(data,size);
    uint8_t loopIteration = 0;

    HTTP_PRINT_LINE_ARGS("IN %s.", __FUNCTION__);
    if (eol == NULL)
    {
        HTTP_PRINT_LINE("Couldn't find EOL");
        return NULL;
    }

    /*TODO LOOP HERE FOR HEADERS */
    if (P_DIFF_LEN(eol, data) <= HTTP_EOL_LEN)
    {
        HTTP_PRINT_LINE("Data length too short to hold EOL characters.");
        return NULL;
    }

    while (P_DIFF_LEN(data, lineStart) <= size)
    {
        memset(&header, 0, sizeof(header));
        /* Check for blank line */
        if (strncmp(data, HTTP_EOL_STR, P_DIFF_LEN(eol,data)) == 0)
        {
            return data;    /* pointer to newline seperating body */
        }

        /* Must be a header then. Get field. */
        header.field.data = data;
        while (data < eol) /* Ensure a byte for header value as well.*/
        {
            if (*data == ':')
            {
                header.field.size = (data-lineStart); /* Length to, not incl the colon */
                break;
            }
            data++;
        }

        if (header.field.size == 0)
        {
            HTTP_PRINT_LINE("header field length was zero.");
            return NULL;
        }
        
        /* data is still sitting on the colon */
        /* expecting at least two more chars - space and value */
        if (data+2 >= eol)
        {
            HTTP_PRINT_LINE("Not enough data to eol.");
            return NULL;
        }

        data++; /* Move data to space */
        if (*data != ' ')
        {
            HTTP_PRINT_LINE("Space not present.");
            return NULL;
        }
        data++;

        /* data is on the header value */
        if (eol==data)
        {
            HTTP_PRINT_LINE("data and eol are the same.");
            return NULL;
        }
     
        header.value.data = data;
        header.value.size = eol-data;   

        /* Only store if room */
        if (loopIteration<MAX_HEADERS)
        {
            par->headers[loopIteration] = header;
        }

        HTTP_PRINT_LINE_ARGS("ALAN DEBUG. Going to try to grab next line."
                        "dataStart: %p size: %d eol: %p dataEnd %p", dataStart,
                        size, eol, dataStart + size);

        /*  TODO problem here. Need to identify when we hit CRLF seperating 
         *  body e.g.*/
        if (dataStart + size - 1 == (eol + HTTP_EOL_LEN + HTTP_EOL_LEN - 1))
        {
            break;
        }
        if ((lineStart = getNextLine(eol, P_OFFSET_LEN(dataStart, size, eol)))
                == NULL)
        {
            return NULL;
        }

        if ((eol = getEndOfLine(lineStart, P_OFFSET_LEN(dataStart, size,
                                lineStart))) == NULL)
        {
            return NULL;
        }

        data=lineStart;
        loopIteration++;
    }

   return eol;/* TODO */
}


const char * parseHttp(httpParser * par, const char * data, uint16_t size, void * user)
{
    const char * dataRtnd;
#if 0
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
    HTTP_PRINT("\n\nnparsed: %u\n", (unsigned int)nparsed);

    if (control->parsed.completed)
    {
        control->parsed.method->callback(NULL, NULL, 0, user);
    }
#endif
    if ((dataRtnd = parseHttpStartLine(data, size, par)) == NULL)
    {
        HTTP_PRINT_LINE("Parse Start Line Failed.");
        return NULL;
    }
    
    else if ((dataRtnd = getNextLine(dataRtnd, P_OFFSET_LEN(data, size, 
                                                          dataRtnd))) == NULL)
    {
        HTTP_PRINT_LINE("getNextLine failed.");
        return NULL;
    }

    else if ((dataRtnd = parseHttpHeaders(dataRtnd,
                                          P_OFFSET_LEN(data, size, dataRtnd),
                                          par)) == NULL)
    {
        HTTP_PRINT_LINE("Parse Headers Failed.");
        return NULL;
    }

    return data; /* TODO */

}





