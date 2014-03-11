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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "clarity_api.h"
#include "http.h"
#include "socket.h"


int32_t clarityHttpBuildResponseTextPlain(char * buf,
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

int32_t claritySendInCb(const connectionInformation * conn,
                        const void * data, uint16_t length)
{
    int32_t rtn;

    clarityCC3000ApiLck();
    rtn = send(conn->socket, data, length, 0);
    clarityCC3000ApiUnlck();

    return rtn;
}


int32_t clarityInit(accessPointInformation * accessPointConnection)
{
    clarityMgmtInit(accessPointConnection);
    return 0;
}



