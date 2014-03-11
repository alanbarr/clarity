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

#include "clarity_api.h"
#include "socket.h"

static clarityError convertAddressInfoToNetworkIp(const addressInformation * addrInfo,
                                                  uint32_t * ip)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    if (addrInfo->type == ADDRESS_URL)
    {
        clarityCC3000ApiLck();

        if (gethostbyname(addrInfo->addr.url,
                          strnlen(addrInfo->addr.url, MAX_URL_LEN)
                          ip) < 0)
        {
            rtn = CLARITY_ERROR_CC3000_SOCKET;
        }
        else
        {
            rtn = CLARITY_SUCCESS;
        }

        clarityCC3000ApiUnlck();
    }
    
    else
    {
        *ip = addrInfo->addr.ip;
        rtn = CLARITY_SUCCESS;
    }

    *ip = htonl(*ip);

    return rtn;
}

static clarityError sendHttpRequest(const addressInformation * addrInfo,
                                    char * buf,
                                    uint16_t bufSize,
                                    uint16_t reqSize,
                                    uint16_t * recvSize)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;
    int32_t socket = -1;
    sockaddr_in saddr;
    uint32_t ip;
    int32_t recvBytes;

    if (rtn = (clarityMgmtRegisterProcessStarted()) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    if (rtn = (convertAddressInfoToNetworkIp(addrInfo, &(saddr.sin_addr))
        != CLARITY_SUCCESS))
    {
        return rtn;
    }

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(80U);

    clarityCC3000ApiLck();

    if ((socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }
    else if (connect(socket, &saddr, sizeof(saddr)) == - 1)
    {
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }
    /* TODO XXX send size - wlan_tx_buffer overflow + partial tx??? */
    else if (send(socket, buf, reqSize) != reqSize)
    {
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }
    else if ((recvBytes = recv(socket, buf, bufSize, 0)) == -1)
    {
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }
    else 
    {
        *recvSize = recvBytes;
    }

    if (socket != -1)
    {
        if (closesocket(socket) != 0)
        {
            rtn = CLARITY_ERROR_CC3000_SOCKET;
        }
    }

    clarityCC3000ApiUnlck();

    if (rtn = (clarityMgmtRegisterProcessFinished()) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    return CLARITY_ERROR;
}


/* buf is reused */
clarityError claritySendHttpRequest(const addressInformation * addr,
                                    const httpInformation * http,
                                    char * buf,
                                    uint16_t bufSize)
{
    uint16_t reqSize = 0;
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    /* TODO unsupported*/
    if (http->type == GET)
    {
        return CLARITY_ERROR_UNDEFINED;
    }

    if ((reqSize = httpBuildRequestTextPlain(http, buf, bufSize)) == 0)
    {
        return rtn;
    }

    if ((rtn = sendHttpRequest(addr, buf, bufSize, reqSize)) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    if ((parseHttpResponse(buf, reqSize) != true))
    {
        rtn = CLARITY_ERROR_RESPONSE;
        return rtn;
    }

    return rtn;
}



