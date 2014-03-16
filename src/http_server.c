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

static WORKING_AREA(httpWorkingArea, 1024);

static Thread * httpServerThd = NULL;
static bool killHttpServer = false;
static char rxBuf[1000];

static clarityControlInformation * controlInfo;
static clarityHttpInformation httpInfo;
static int serverSocket;

static Mutex * cc3000Mtx;

void clarityCC3000ApiLck(void)
{ 
    if (cc3000Mtx != NULL)
    {
        chMtxLock(cc3000Mtx);
    }
}

void clarityCC3000ApiUnlck(void)
{
    if (cc3000Mtx != NULL)
    {
        chMtxUnlock();
    }
}

static msg_t cc3000HttpServer(void * arg)
{
    sockaddr acceptedAddr;
    socklen_t acceptedAddrLen = sizeof(acceptedAddr);
    const char * httpRtn = NULL;
    int16_t rxBytes = 0;
    clarityConnectionInformation accepted;
    
    (void)arg;

    while (1)
    {
        CLAR_PRINT("Server: top of while 1", NULL);

        if (killHttpServer == true)
        {
            break;
        }
 
        memset(&acceptedAddr, 0, sizeof(acceptedAddr));
        memset(rxBuf, 0, sizeof(rxBuf));

        while (killHttpServer != true)
        {
            clarityCC3000ApiLck();
            if ((accepted.socket = accept(serverSocket, &acceptedAddr,
                                          &acceptedAddrLen)) < 0)
            {
                if (accepted.socket == -1)
                {
                    CLAR_PRINT("accept() returned error: %d", accepted.socket);
                }
                else
                {
                    clarityCC3000ApiUnlck();
                    chThdSleep(MS2ST(500));
                }
            }

            else
            {
                break;
            }
        }
        
        if ((rxBytes = recv(accepted.socket, rxBuf, sizeof(rxBuf), 0)) == -1)
        {
            CLAR_PRINT("recv() returned error.", NULL);
        }

        clarityCC3000ApiUnlck();

        if (rxBytes > 0)
        {
            if ((httpRtn = httpRequestProcess(controlInfo,
                                              &httpInfo, 
                                              &accepted,
                                              rxBuf, rxBytes)) == NULL)
            {
                CLAR_PRINT("httpRequestProcess() returned NULL.", NULL);
            }
        }
 
        clarityCC3000ApiLck();
        if (closesocket(accepted.socket) == -1)
        {
            CLAR_PRINT("closesocket() failed for acceptedSocket.", NULL);
        }
        clarityCC3000ApiUnlck();

        chThdSleep(MS2ST(500));
    }

    clarityCC3000ApiLck();
    if (closesocket(serverSocket) == -1)
    {
        CLAR_PRINT("closesocket() failed for serverSocket.", NULL);
    }
    clarityCC3000ApiUnlck();
 
    if (clarityMgmtRegisterProcessFinished() != CLARITY_SUCCESS)
    {
        /* TODO */
    }

    return 0;
}

clarityError clarityHttpServerKill(void)
{
    killHttpServer = true;
    /* TODO ensure server ends */
    return CLARITY_SUCCESS;
}

clarityError clarityHttpServerStart(Mutex * cc3000ApiMtx,
                                    clarityControlInformation * control)
{
    uint16_t blockOpt = SOCK_ON;
    clarityError rtn = CLARITY_ERROR_UNDEFINED;
    sockaddr_in serverAddr = {0};

    cc3000Mtx = cc3000ApiMtx;
    controlInfo = control;

    memset(rxBuf, 0, sizeof(rxBuf));

    if ((rtn = clarityMgmtRegisterProcessStarted()) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
        return rtn;
    }

    /* We need dhcp info here */
    if (cc3000AsyncData.dhcp.present != 1)
    {
        CLAR_PRINT("No DHCP info present", NULL);

        if ((clarityMgmtRegisterProcessFinished()) != CLARITY_SUCCESS)
        {
            CLAR_PRINT_ERROR();
            /* TODO */
        }
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);
#if 0
    serverAddr.sin_addr.s_addr = *(int *)cc3000AsyncData.dhcp.info.aucIP; 
#endif
    CLAR_PRINT("ALAN DEBUG DHCP ADDR IS %x", serverAddr.sin_addr.s_addr);


    clarityCC3000ApiLck();

    if ((serverSocket = socket(AF_INET, SOCK_STREAM,
                               IPPROTO_TCP)) == -1)
    {
        CLAR_PRINT("socket() returned error.", NULL);
        while(1);
    }
    else if (setsockopt(serverSocket, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK,
                        &blockOpt, sizeof(blockOpt)) == -1)
    {
        CLAR_PRINT("setsockopt() returned error.", NULL);
        while(1);
    }

    else if (bind(serverSocket, (sockaddr *)&serverAddr,
                  sizeof(serverAddr)) == -1)
    {
        serverSocket = -1;
        CLAR_PRINT("bind() returned error.", NULL);
        while(1);
    }

    else if (listen(serverSocket, 1) != 0)
    {
        serverSocket = -1;
        CLAR_PRINT("bind() returned error.", NULL);
        while(1);
    }
     
    clarityCC3000ApiUnlck();

    if (serverSocket != -1)
    {
        httpServerThd = chThdCreateStatic(httpWorkingArea,
                                          sizeof(httpWorkingArea),
                                          NORMALPRIO,
                                          cc3000HttpServer, NULL);
    }

    return CLARITY_SUCCESS;
}


