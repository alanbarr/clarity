#include <stdio.h>
#include "http.h"
#include <stdint.h>


int32_t clarityHttpResponseTextPlain(char * buf,
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
