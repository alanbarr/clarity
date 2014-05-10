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
#include <string.h>
#include "ch.h"
#include "cc3000_chibios_api.h"
#include "socket.h"
#include "clarity_api.h"
#include "clarity_int.h"

#define HTTP_PORT 80
#define SERVER_SLEEP_MS 200

static WORKING_AREA(httpWorkingArea, 1024);

static Thread * httpServerThd = NULL;
static char rxBuf[1000];

static clarityHttpServerInformation * controlInfo;
static clarityHttpRequestInformation httpInfo;
static int serverSocket;

static msg_t cc3000HttpServerThd(void * arg)
{
    sockaddr acceptedAddr;
    socklen_t acceptedAddrLen = sizeof(acceptedAddr);
    const char * httpRtn = NULL;
    int16_t rxBytes = 0;
    clarityConnectionInformation accepted;
    
    (void)arg;

    while (chThdShouldTerminate() == FALSE)
    {
        uint32_t timeout = 5000; /* ms */

        memset(&acceptedAddr, 0, sizeof(acceptedAddr));
        memset(rxBuf, 0, sizeof(rxBuf));

        while (chThdShouldTerminate() == FALSE)
        {
            clarityCC3000ApiLock();
            if ((accepted.socket = accept(serverSocket, &acceptedAddr,
                                          &acceptedAddrLen)) < 0)
            {

                clarityCC3000ApiUnlock();
                if (accepted.socket == -1)
                {
                    CLAR_PRINT_ERROR();
                }
                else
                {
                    chThdSleep(MS2ST(SERVER_SLEEP_MS));
                }
            }

            else
            {
                if (setsockopt(accepted.socket, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT,
                               &timeout, sizeof(timeout)) != 0)
                {
                    CLAR_PRINT_ERROR();
                }

                clarityCC3000ApiUnlock();
                break;
            }
        }
  
        if (chThdShouldTerminate())
        {
            break;
        }

        clarityCC3000ApiLock();
        if ((rxBytes = recv(accepted.socket, rxBuf, sizeof(rxBuf), 0)) == -1)
        {
            CLAR_PRINT_ERROR();
        }

        clarityCC3000ApiUnlock();

        if (rxBytes > 0)
        {
            if ((httpRtn = httpRequestProcess(controlInfo,
                                              &httpInfo, 
                                              &accepted,
                                              rxBuf, rxBytes)) == NULL)
            {
                CLAR_PRINT_ERROR();
            }
        }
 
        if (accepted.socket > 0)
        {
            clarityCC3000ApiLock();
            if (closesocket(accepted.socket) == -1)
            {
                CLAR_PRINT_ERROR();
            }
            clarityCC3000ApiUnlock();
        }
        chThdSleep(MS2ST(SERVER_SLEEP_MS));
    }

    clarityCC3000ApiLock();
    if (closesocket(serverSocket) == -1)
    {
        CLAR_PRINT_ERROR();
    }
    clarityCC3000ApiUnlock();
 
    if (clarityRegisterProcessFinished() != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
    }

    return CLARITY_SUCCESS;;
}

clarityError clarityHttpServerStop(void)
{
    if (httpServerThd != NULL)
    {
        chThdTerminate(httpServerThd);
        chThdWait(httpServerThd);
        httpServerThd = NULL;
    }
    return CLARITY_SUCCESS;
}

clarityError clarityHttpServerStart(clarityHttpServerInformation * control)
{
    uint16_t blockOpt = SOCK_ON;
    clarityError rtn = CLARITY_ERROR_UNDEFINED;
    sockaddr_in serverAddr = {0};
    controlInfo = control;

    memset(rxBuf, 0, sizeof(rxBuf));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(HTTP_PORT);

    if ((rtn = clarityRegisterProcessStarted()) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
        return rtn;
    }

    /* We need dhcp info here */
    if (cc3000AsyncData.dhcp.present != 1)
    {
        CLAR_PRINT("No DHCP info present", NULL);

        if ((clarityRegisterProcessFinished()) != CLARITY_SUCCESS)
        {
            CLAR_PRINT_ERROR();
        }

        return CLARITY_ERROR_UNDEFINED;
    }

    clarityCC3000ApiLock();

    if ((serverSocket = socket(AF_INET, SOCK_STREAM,
                               IPPROTO_TCP)) == -1)
    {
        CLAR_PRINT("socket() returned error.", NULL);
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    else if (setsockopt(serverSocket, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK,
                        &blockOpt, sizeof(blockOpt)) == -1)
    {
        CLAR_PRINT("setsockopt() returned error.", NULL);
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    else if (bind(serverSocket, (sockaddr *)&serverAddr,
                  sizeof(serverAddr)) == -1)
    {
        serverSocket = -1;
        CLAR_PRINT("bind() returned error.", NULL);
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    else if (listen(serverSocket, 1) != 0)
    {
        serverSocket = -1;
        CLAR_PRINT("bind() returned error.", NULL);
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }
     
    clarityCC3000ApiUnlock();

    if (serverSocket != -1)
    {
        httpServerThd = chThdCreateStatic(httpWorkingArea,
                                          sizeof(httpWorkingArea),
                                          NORMALPRIO,
                                          cc3000HttpServerThd, NULL);
    }

    return rtn;
}

clarityError clarityHttpServerSendInCb(const clarityConnectionInformation * conn,
                                       const void * data, uint16_t length)
{
    int32_t rtn;

    clarityCC3000ApiLock();
    rtn = send(conn->socket, data, length, 0);
    clarityCC3000ApiUnlock();

    return rtn;
}


