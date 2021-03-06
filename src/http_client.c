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
#include "clarity_api.h"
#include "clarity_int.h"
#include "http.h"
#include "socket.h"

/* N.B. edits addrInfo 
 * ip returned in network byte order */
static clarityError convertAddressInfoToNetworkIp(clarityAddressInformation * addrInfo,
                                                  uint32_t * ip)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    if (addrInfo->type == CLARITY_ADDRESS_URL)
    {
        clarityCC3000ApiLock();

        if (gethostbyname(addrInfo->addr.url,
                          strnlen(addrInfo->addr.url, CLARITY_MAX_URL_LENGTH),
                          ip) < 0)
        {
            rtn = CLARITY_ERROR_CC3000_SOCKET;
        }
        else
        {
            /* XXX edit transport to prevent further lookups */
            addrInfo->type = CLARITY_ADDRESS_IP;
            addrInfo->addr.ip = *ip;

            rtn = CLARITY_SUCCESS;
        }

        clarityCC3000ApiUnlock();
    }
    
    else
    {
        *ip = addrInfo->addr.ip;
        rtn = CLARITY_SUCCESS;
    }

    *ip = htonl(*ip);

    return rtn;
}
static clarityError sendHttpRequest(clarityTransportInformation * transport,
                                    clarityHttpPersistant * persistant,
                                    char * buf,
                                    uint16_t bufSize,
                                    uint16_t reqSize,
                                    uint16_t * recvSize)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;
    int32_t sockfd = -1;
    sockaddr_in saddr;
    int32_t recvBytes;
    int32_t recvAttempt;

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(transport->port);

    if (persistant != NULL && persistant->connected == false &&
        ((rtn = clarityRegisterProcessStarted()) != CLARITY_SUCCESS))
    {
        return rtn;
    }

    else if ((rtn = convertAddressInfoToNetworkIp(&(transport->addr), 
                                                  &(saddr.sin_addr.s_addr)))
                  != CLARITY_SUCCESS)
    {
        return rtn;
    }

    clarityCC3000ApiLock();

    if (persistant == NULL || persistant->connected == false) /* XXX */
    {
        uint32_t timeout = 1000; /* ms */ /* TODO make global */

        if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
            CLAR_PRINT_ERROR();
            rtn = CLARITY_ERROR_CC3000_SOCKET;
        }

        else if (connect(sockfd, (sockaddr*)&saddr, sizeof(saddr)) == - 1)
        {
            CLAR_PRINT_ERROR();
            rtn = CLARITY_ERROR_CC3000_SOCKET;
        }
        else if (setsockopt(sockfd, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT,
                            &timeout, sizeof(timeout)) != 0)
        {
            CLAR_PRINT_ERROR();
        }
        else if (persistant != NULL)
        {
            persistant->connected = true;
        }

    }
    else
    {
        sockfd = persistant->socket;
    }

    if (rtn == CLARITY_ERROR_CC3000_SOCKET)
    {

    }

    /* TODO XXX send size - wlan_tx_buffer overflow + partial tx??? */
    else if (send(sockfd, buf, reqSize, 0) != reqSize)
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }
    
    clarityCC3000ApiUnlock();

    /* TODO tidy up. recv non-block ??? */
    for(recvAttempt=0; recvAttempt<10; recvAttempt++)
    {
        clarityCC3000ApiLock();
        if ((recvBytes = recv(sockfd, buf, bufSize, 0)) == -1)
        {
            CLAR_PRINT_ERROR();
            clarityCC3000ApiUnlock();
            break;
        }
        else if (recvBytes > 0)
        {
            clarityCC3000ApiUnlock();
            break;
        }
        clarityCC3000ApiUnlock();
    }

    if (recvBytes>0)
    {
        *recvSize = recvBytes;
    }
    else 
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    if (sockfd != -1)
    {
        if (persistant != NULL)
        {
            if (persistant->connected == false)
            {
                persistant->connected = true;
                persistant->socket = sockfd;
            }

        }
        if (persistant == NULL || persistant->closeOnComplete == true)
        {
            clarityCC3000ApiLock();
            if (closesocket(sockfd) != 0)
            {
                CLAR_PRINT_ERROR();
                rtn = CLARITY_ERROR_CC3000_SOCKET;
            }
            clarityCC3000ApiUnlock();

            if (persistant != NULL)
            {
                persistant->connected = false;
                persistant->socket = -1;
            }
        }
    }


    if (persistant == NULL || persistant->closeOnComplete == true)
    {
        clarityRegisterProcessFinished();
    }

    return rtn;
}


# if 0
/* buf is reused 
 * TODO get addr to a const*/
clarityError clarityBuildSendHttpRequest(clarityTransportInformation * transport,
                                         const clarityHttpRequestInformation * request,
                                         char * buf,
                                         uint16_t bufSize,
                                         clarityHttpResponseInformation * response)
{
    uint16_t reqSize = 0;
    clarityError rtn = CLARITY_ERROR_UNDEFINED;
    uint16_t recvSize;

    /* TODO unsupported*/
    if (request->type == GET)
    {
        return CLARITY_ERROR_UNDEFINED;
    }

    if ((reqSize = httpBuildRequestTextPlain(request, buf, bufSize)) == 0)
    {
        return rtn;
    }

    if ((rtn = sendHttpRequest(transport, buf, bufSize, reqSize, &recvSize)) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    if (httpParseResponse(response, buf, recvSize) == NULL)
    {
        rtn = CLARITY_ERROR_REMOTE_RESPONSE;
        return rtn;
    }

    return rtn;
}
#endif


/* buf is reused 
 * TODO get addr to a const*/
clarityError clarityHttpSendRequest(clarityTransportInformation * transport,
                                    clarityHttpPersistant * persistant,
                                    char * buf,
                                    uint16_t bufSize,
                                    uint16_t requestSize,
                                    clarityHttpResponseInformation * response)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;
    uint16_t recvSize;

    if (transport->type != CLARITY_TRANSPORT_TCP)
    {
        CLAR_PRINT_ERROR();
        return CLARITY_ERROR_UNDEFINED;
        /* XXX */
    }

    if (requestSize > bufSize)
    {
        CLAR_PRINT_ERROR();
        return CLARITY_ERROR_RANGE;
    }

    if ((rtn = sendHttpRequest(transport, persistant, buf, bufSize, requestSize, &recvSize))
             != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
        return rtn;
    }

    /* TODO what about continues */
    if (httpParseResponse(response, buf, recvSize) == NULL)
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_REMOTE_RESPONSE;
        return rtn;
    }

    return rtn;
}

clarityError clarityHttpBuildPost(char * buf, uint16_t bufSize,
                                  const char * device,   /* null t */
                                  const char * resource, /* null t */
                                  const char * content,   /* null t */
                                  clarityHttpPersistant * persistant)
{
    int16_t temp = 0;
    
    temp = snprintf(buf, bufSize - temp, "POST %s%s " 
                                          HTTP_VERSION_STR
                                          HTTP_EOL_STR, device, resource); 
    temp += snprintf(buf + temp, bufSize - temp, "Host:" HTTP_EOL_STR); /* hacky hack */
        
    if (persistant == NULL || persistant->closeOnComplete == true)
    {
        temp += snprintf(buf + temp, bufSize - temp, "Connection: close" HTTP_EOL_STR); 
    }

    if (content != NULL)
    {
        temp += snprintf(buf + temp, bufSize - temp, "Content-Type: text/plain"
                                                     HTTP_EOL_STR);
        temp += snprintf(buf + temp, bufSize - temp, "Content-Length: %d"
                                                     HTTP_EOL_STR, strlen(content));
        temp += snprintf(buf + temp, bufSize - temp, HTTP_EOL_STR);
        temp += snprintf(buf + temp, bufSize - temp, "%s", content);
    }
    else 
    {
        temp += snprintf(buf, bufSize - temp, "%s", content);
    }
    return temp;
}
