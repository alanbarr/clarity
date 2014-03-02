#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "clarity_api.h"
#include "http_parser.h"

#ifdef METHOD
    #undef METHOD
#endif 
#define METHOD(M)    #M,
static const char * methodStrings[] = {
    METHODS
    "INVALID"
};

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


static const char * parseStartLine(const char * data,
                                   uint16_t size,
                                   httpInformation * info)
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
    
    for (type = 0; type < METHOD_TYPE_MAX; type++)
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

    if (type == METHOD_TYPE_MAX)
    {
        HTTP_PRINT_LINE("FAIL");
        return NULL;
    }

    else if (strncasecmp(c, methodStrings[type],
                         SMALLER(strlen(methodStrings[type]), (eol-c)) != 0))
    {
        HTTP_PRINT_LINE("FAIL");
        return NULL;
    }

    info->type = type;

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
        if (isdigit((int)*c) == 0)
        {
            HTTP_PRINT_LINE("Version Major was not a digit.");
            return NULL;
        }
        else
        {
            (info->version.major = *c - 48);
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
        if (isdigit((int)*c) == 0)
        {
            HTTP_PRINT_LINE("Version Minor not a digit.");
            return NULL;
        }
        else
        {
            (info->version.minor = *c - ASCII_DIGIT_OFFSET);
        }
    }

    /* Update Resource */
    info->resource.data = resource;
    info->resource.size = version - resource - HTTP_SPACE_LEN;

    /* Success */
    return eol; /* todo range */
}


/* data is pointer to first byte on a new line */
/* TODO content length probably important to nab here. maybe content type as well */
/* @return NULL on error. On success it returns a pointer to the first byte
 *         in the blank seperator line. */
static const char * parseHeaders(const char * data,
                                 uint16_t size,
                                 httpInformation * info)
{
    const char * const dataStart = data;
    header header;
    const char * lineStart = data;
    const char * eol = getEndOfLine(data,size);
    uint8_t headersIndex = 0;

    HTTP_PRINT_LINE_ARGS("IN %s.", __FUNCTION__);
    if (eol == NULL)
    {
        HTTP_PRINT_LINE("Couldn't find EOL");
        return NULL;
    }

#if 0
    if (P_DIFF_LEN(eol, data) < HTTP_EOL_LEN)
    {
        HTTP_PRINT_LINE("Data length too short to hold EOL characters.");
        return NULL;
    }
#endif

    while (P_DIFF_LEN(data, lineStart) <= size)
    {
        memset(&header, 0, sizeof(header));
 
        /* TODO IS THIS RIGHT - stop when we hit body. */
        /* body seperator */
        if (eol == lineStart)
        {
            return eol; /* Only valid return */
        }

#if 0
        /* Check for blank line */ /* TODO what is this doing? redundant by the above? */
        if (strncmp(data, HTTP_EOL_STR, P_DIFF_LEN(eol,data)) == 0)
        {
            return data;    /* pointer to newline seperating body */
        }
#endif

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

        /* Store the header as a generic header unless its content- related */
        if (strncasecmp(header.field.data, HTTP_CONTENT_TYPE_STR,
                        SMALLER(HTTP_CONTENT_TYPE_LEN, header.field.size)) == 0)
        {
            info->content = header.value;
        }

        else if (strncasecmp(header.field.data, HTTP_CONTENT_LENGTH_STR,
                             SMALLER(HTTP_CONTENT_LENGTH_LEN, header.field.size))
                 == 0)
        {
            long length;
            char * end;
            char temp[MAX_CONTENT_LENGTH_DIGITS];

            if (header.value.size >= MAX_CONTENT_LENGTH_DIGITS)
            {
                return NULL;
            }
            
            memset(temp,0,sizeof(temp));
            memcpy(temp, header.value.data, header.value.size);
            temp[MAX_CONTENT_LENGTH_DIGITS-1] = 0; /* Force terminator */

            errno = 0;

            length = strtol(temp, &end, 10);

            if (end != NULL && *end != 0)
            {
                return NULL;
            }

            if (length == LONG_MAX || length == LONG_MIN || length == 0)
            {
                if (errno == EINVAL || errno == ERANGE)
                {
                    return NULL;
                }
            }

            if (length > UINT16_MAX)
            {
                return NULL;
            }
            
            info->body.size = length; /* TODO needs validated later */
        }

        /* Only store if room */
        else if (headersIndex<MAX_HEADERS)
        {
            info->headers[headersIndex] = header;
            headersIndex++;
        }

        HTTP_PRINT_LINE_ARGS("ALAN DEBUG. Going to try to grab next line."
                        "dataStart: %p size: %d eol: %p dataEnd %p", dataStart,
                        size, eol, dataStart + size);

#if 0
        /*  TODO problem here. Need to identify when we hit CRLF seperating 
         *  body e.g.
         *  ALAN TODO THIS IS WROMG. NEED TO LOOK FOR A LINE WITH ONLY A a CRLF*/
        if (dataStart + size - 1 == (eol + HTTP_EOL_LEN + HTTP_EOL_LEN - 1))
        {
            break;
        }
#endif

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
    }

   return NULL;
}


/* TODO assuming data point to the blank line */
/* @return NULL on errror. Upon success it points to the last byte contained
 *         in the body */
static const char * parseBody(const char * data,
                                  uint16_t size,
                                  httpInformation * info)
{
    const char * body = NULL;

    /* not enough data for blank line */
    if (size < HTTP_EOL_LEN)
    {
        return NULL;
    }
    
    if (size - HTTP_EOL_LEN < info->body.size)
    {   
        info->body.size = 0;
        return NULL;
    }

    /* No body */
    if (size == HTTP_EOL_LEN)
    {
        return data;
    }

    if ((body = getNextLine(data, size)) == NULL)
    {
        return NULL;
    }

    info->body.data = body;

    return body + info->body.size;
}

static const char * httpParseRequest(httpInformation * info, const char * data,
                                     uint16_t size)
{
    const char * dataRtnd = NULL;

    if ((dataRtnd = parseStartLine(data, size, info)) == NULL)
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

    else if ((dataRtnd = parseHeaders(dataRtnd,
                                          P_OFFSET_LEN(data, size, dataRtnd),
                                          info)) == NULL)
    {
        HTTP_PRINT_LINE("Parse Headers Failed.");
        return NULL;
    }
     
    /* Has a body been reported ?*/
    if (info->body.size)
    {
        if ((dataRtnd = parseBody(dataRtnd, P_OFFSET_LEN(data, size, dataRtnd),
                                           info)) == NULL)
        {
            HTTP_PRINT_LINE("Parse Data Failed.");
            return NULL;
        }
        else
        {
            return dataRtnd;
        }
    }
    else 
    {
        /* No body. Sitting on the blank line at the end of headers. */
        return dataRtnd+HTTP_EOL_LEN-1;
    }
}

static const char * httpParseResponse(httpInformation * info, const char * data,
                                      uint16_t size)
{
    (void)info;
    (void)data;
    (void)size;
    return NULL;
}


const char * httpParse(httpInformation * info, const char * data, uint16_t size)
{
    unsigned char first = 0;
    unsigned char second = 0;
 
    memset(info, 0, sizeof(*info));

    if (size<2)
    {
        return NULL;
    }
    
    first = toupper((int)*data);

    switch (first)
    {
        case 'G':
        case 'P':
        case 'T':
        case 'O':
        case 'D':
            return httpParseRequest(info,data,size);
        case 'H':
            second = toupper((int)*(data+1));
            if (second == 'E')
            {
                return httpParseRequest(info,data,size);
            }
            else
            {
                return httpParseResponse(info,data,size);
            }
        default:
            return NULL;
    }
    
    return NULL; /* TODO */
}


