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

#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "clarity_api.h"
#include "http.h"

#ifdef CLARITY_HTTP_METHOD
    #undef CLARITY_HTTP_METHOD
#endif 
#define CLARITY_HTTP_METHOD(M)    #M,
static const char * methodStrings[] = {
    CLARITY_HTTP_METHODS
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
        CLAR_PRINT_LINE("getEndOfLine failed.");
        return NULL;
    }

    if ((dataRtnd + HTTP_EOL_LEN) > (data + size - 1))
    {
        CLAR_PRINT_LINE("Not enough room for EOL.");
        return NULL;
    }
    
    CLAR_PRINT_LINE("Should be returning success.");
    return (dataRtnd += HTTP_EOL_LEN);
}


/* Return pointer to last char of version */
static const char * getHttpVersion(const char * version,
                                   const char * eol,
                                   uint8_t * major,
                                   uint8_t * minor)
{
    const char * c = version;

    if (eol - version < HTTP_VERSION_LEN)
    {
        return NULL;
    }
 
    if (strncasecmp(version, "HTTP/", 5) != 0)  /* TODO */
    {
        CLAR_PRINT_LINE("HTTP/ not present.");
        return NULL;
    }

    c += 5;                       /* Skip HTTP/ */
 
    /* First Digit */
    if (isdigit((int)*c) == 0)
    {
        CLAR_PRINT_LINE("Version Major was not a digit.");
        return NULL;
    }
    else
    {
        (*major= *c - 48);
    }

    /* Check for dp */
    c++;                /* You'll find no OOP here */
    if (*c != '.')
    {
        CLAR_PRINT_LINE("Version DP not as expected.");
        return NULL;
    }

    /* Second Digit */
    c++;
    if (isdigit((int)*c) == 0)
    {
        CLAR_PRINT_LINE("Version Minor not a digit.");
        return NULL;
    }
    else
    {
        (*minor = *c - ASCII_DIGIT_OFFSET);
    }

    return c;
}


static const char * parseResponseStartLine(const char * data,
                                          uint16_t size,
                                          clarityHttpResponseInformation * info)
{
    const char * c = data;
    const char * eol = getEndOfLine(data, size);
    int32_t tempCode;
    char temp[HTTP_STATUS_CODE_DIGITS + 1];
    char * end = NULL;

    if (eol == NULL)
    {
        CLAR_PRINT_LINE("Could not find eol.");
        return NULL;
    }

    if ((eol-c) < HTTP_RES_START_LINE_MIN_LEN)
    {
        CLAR_PRINT_LINE_ARGS("eol: %p c: %p line length: %d", eol,c,(int)(eol-c));
        CLAR_PRINT_LINE("FAIL");
        return NULL;
    }
    
    if ((c = getHttpVersion(c, eol, &(info->version.major),
                            &(info->version.minor))) == NULL)
    {
        return NULL;
    }

    c++;
    if (*c != ' ')
    {
        return NULL;
    }
    c++; /* Now should be on code */
    
    memcpy(temp, c, HTTP_STATUS_CODE_DIGITS);
    temp[HTTP_STATUS_CODE_DIGITS] = 0;
    
    errno = 0;
    tempCode = strtol(temp, &end, 10);

    if (end != NULL && *end != 0)
    {
        return NULL;
    }
    if (tempCode == LONG_MAX || tempCode == LONG_MIN || tempCode == 0)
    {
        if (errno == EINVAL || errno == ERANGE)
        {
            return NULL;
        }
    }
    if (tempCode > UINT16_MAX)
    {
        return NULL;
    }
    
    info->code = tempCode;

    /* TODO String */

    /* Success */
    return eol; /* todo range */
}


/* data is pointer to first byte on a new line */
/* TODO content length probably important to nab here. maybe content type as well */
/* @return NULL on error. On success it returns a pointer to the first byte
 *         in the blank seperator line. */
static const char * parseHeaders(const char * data,
                                 uint16_t size,
                                 clarityHttpRequestInformation * info)
{
    const char * const dataStart = data;
    clarityHttpHeader header;
    const char * lineStart = data;
    const char * eol = getEndOfLine(data,size);
    uint8_t headersIndex = 0;

    CLAR_PRINT_LINE_ARGS("IN %s.", __FUNCTION__);
    if (eol == NULL)
    {
        CLAR_PRINT_LINE("Couldn't find EOL");
        return NULL;
    }

#if 0
    if (P_DIFF_LEN(eol, data) < HTTP_EOL_LEN)
    {
        CLAR_PRINT_LINE("Data length too short to hold EOL characters.");
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
            CLAR_PRINT_LINE("header field length was zero.");
            return NULL;
        }
        
        /* data is still sitting on the colon */
        /* expecting at least two more chars - space and value */
        if (data+2 >= eol)
        {
            CLAR_PRINT_LINE("Not enough data to eol.");
            return NULL;
        }

        data++; /* Move data to space */
        if (*data != ' ')
        {
            CLAR_PRINT_LINE("Space not present.");
            return NULL;
        }
        data++;

        /* data is on the header value */
        if (eol==data)
        {
            CLAR_PRINT_LINE("data and eol are the same.");
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
            char temp[CLARITY_MAX_CONTENT_LENGTH_DIGITS];

            if (header.value.size >= CLARITY_MAX_CONTENT_LENGTH_DIGITS)
            {
                return NULL;
            }
            
            memset(temp,0,sizeof(temp));
            memcpy(temp, header.value.data, header.value.size);
            temp[CLARITY_MAX_CONTENT_LENGTH_DIGITS-1] = 0; /* Force terminator */

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
        else if (headersIndex < CLARITY_MAX_HEADERS)
        {
            info->headers[headersIndex] = header;
            headersIndex++;
        }

        CLAR_PRINT_LINE_ARGS("ALAN DEBUG. Going to try to grab next line."
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
                                  clarityHttpRequestInformation * info)
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
 
static const char * parseRequestStartLine(const char * data,
                                          uint16_t size,
                                          clarityHttpRequestInformation * info)
{
    const char * c = data;
    const char * resource = NULL;
    const char * version = NULL;
    clarityHttpMethodType type = 0;
    const char * eol = getEndOfLine(data, size);

    if (eol == NULL)
    {
        CLAR_PRINT_LINE("Could not find eol.");
        return NULL;
    }

    if ((eol-c) < HTTP_REQ_START_LINE_MIN_LEN)
    {
        CLAR_PRINT_LINE_ARGS("eol: %p c: %p line length: %d", eol,c,(int)(eol-c));
        CLAR_PRINT_LINE("FAIL");
        return NULL;
    }
    
    for (type = 0; type < CLARITY_HTTP_METHOD_MAX; type++)
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

    if (type == CLARITY_HTTP_METHOD_MAX)
    {
        CLAR_PRINT_LINE("FAIL");
        return NULL;
    }

    else if (strncasecmp(c, methodStrings[type],
                         SMALLER(strlen(methodStrings[type]), (eol-c)) != 0))
    {
        CLAR_PRINT_LINE("FAIL");
        return NULL;
    }

    info->type = type;

    c += strlen(methodStrings[type]);
    
    if (*c != ' ')
    {
        CLAR_PRINT_LINE("Space not found after type.");
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
        CLAR_PRINT_LINE("Resource string did not start as expected.");
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
        CLAR_PRINT_LINE("Version was NULL.");
        return NULL;
    }
    
    if ((c = getHttpVersion(version, eol, &(info->version.major),
                            &(info->version.minor))) == NULL)
    {
        return NULL;
    }
    /* Update Resource */
    info->resource.data = resource;
    info->resource.size = version - resource - HTTP_SPACE_LEN;

    /* Success */
    return eol; /* todo range */
}

const char * httpParseRequest(clarityHttpRequestInformation * info, const char * data,
                              uint16_t size)
{
    const char * dataRtnd = NULL;
    
    memset(info, 0, sizeof(*info));

    if ((dataRtnd = parseRequestStartLine(data, size, info)) == NULL)
    {
        CLAR_PRINT_LINE("parseRequestStartLine() Failed.");
        return NULL;
    }
    
    else if ((dataRtnd = getNextLine(dataRtnd, P_OFFSET_LEN(data, size, 
                                                          dataRtnd))) == NULL)
    {
        CLAR_PRINT_LINE("getNextLine() failed.");
        return NULL;
    }

    else if ((dataRtnd = parseHeaders(dataRtnd,
                                          P_OFFSET_LEN(data, size, dataRtnd),
                                          info)) == NULL)
    {
        CLAR_PRINT_LINE("parseHeaders() Failed.");
        return NULL;
    }
     
    /* Has a body been reported ?*/
    if (info->body.size)
    {
        if ((dataRtnd = parseBody(dataRtnd, P_OFFSET_LEN(data, size, dataRtnd),
                                           info)) == NULL)
        {
            CLAR_PRINT_LINE("Parse Data Failed.");
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

const char * httpParseResponse(clarityHttpResponseInformation * info,
                               const char * data,
                               uint16_t size)
{
    const char * dataRtnd = NULL;
    if ((dataRtnd = parseResponseStartLine(data, size, info)) == NULL)
    {
        CLAR_PRINT_LINE("parseResponseStartLine() failed.");
        return NULL;
    }
    return NULL;
}



/* builds a http 1.0 request */
uint16_t httpBuildRequestTextPlain(const clarityHttpRequestInformation * http,
                                   char * txBuf,
                                   uint16_t txBufSize)
{
#define HTTP_REQ_LEFT      (txBufSize - bufIndex)
#define HTTP_REQ_CURRENT   (txBuf + bufIndex)
    int16_t bufIndex = 0;
    int8_t headerIndex = 0;

    /* First Line */
    bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT, "%s ",
                         methodStrings[http->type]);
    bufIndex += snprintf(HTTP_REQ_CURRENT, 
                         SMALLER(HTTP_REQ_LEFT, http->resource.size),
                         "%s ", http->resource.data);
    bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT, 
                         "%s", HTTP_VERSION_STR /* TODO ignoring whats in the struct? */);
    bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT,"%s", HTTP_EOL_STR);

    /* Headers */
    while (headerIndex < CLARITY_MAX_HEADERS)
    {
        const clarityHttpHeader * header = &http->headers[headerIndex];
        
        if (header->field.size == 0)
        {
            break;
        }
    
        bufIndex += snprintf(HTTP_REQ_CURRENT,
                             SMALLER(HTTP_REQ_LEFT, header->field.size),
                             "%s: ", 
                             header->field.data);
        bufIndex += snprintf(HTTP_REQ_CURRENT,
                             SMALLER(HTTP_REQ_LEFT, header->value.size),
                             "%s", 
                             header->value.data);
        bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT,"%s", HTTP_EOL_STR);
        headerIndex++;
    }
    if (http->body.size != 0)
    {
        bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT,
                             "%s text/plain", HTTP_CONTENT_TYPE_STR); /* TODO XXX*/
        bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT,"%s", HTTP_EOL_STR);

        bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT,
                             "%s %d", HTTP_CONTENT_LENGTH_STR, http->body.size); /* TODO for all??? */
        bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT,"%s", HTTP_EOL_STR);
    }

    bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT,"%s", HTTP_EOL_STR);
    bufIndex += snprintf(HTTP_REQ_CURRENT, HTTP_REQ_LEFT, "%s", http->body.data);

    /* TODO error checking */
    
    if (bufIndex == txBufSize)
    {
        return 0; /* Assume error */
    }
    return txBufSize;
#undef HTTP_REQ_LEFT
#undef HTTP_REQ_CURRENT

}

clarityError clarityHttpBuildResponseTextPlain(char * buf,
                                          uint16_t bufSize,
                                          uint8_t code,
                                          const char * message,
                                          const char * bodyString)
{
    
#define HTTP_RESPONSE_LEFT      (bufSize - bufIndex)
#define HTTP_RESPONSE_CURRENT   (buf + bufIndex)

    int32_t bufIndex = 0;
    
    /* Create HTTP Response */
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, "%s ", HTTP_VERSION_STR);
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, "%3u ", code);
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, "%s ", message);
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, HTTP_EOL_STR);
    
    /* Headers */
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, "Content-type: text/plain");
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, HTTP_EOL_STR);

    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, "Content-length: ");
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, "%d", strlen(bodyString));
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, HTTP_EOL_STR);

    /* Empty line - body */
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, HTTP_EOL_STR);
    bufIndex += snprintf(HTTP_RESPONSE_CURRENT, HTTP_RESPONSE_LEFT, "%s", bodyString);

    if (bufIndex < bufSize - 1)
    {
        buf[bufIndex] = 0;
    }

#undef HTTP_RESPONSE_LEFT   
#undef HTTP_RESPONSE_CURRENT 

    return bufIndex;
}

