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

#include "socket.h"
#include "clarity_api.h"
#include "cc3000_chibios_api.h"
#include "string.h"
#include "fyp.h"
#include <stdio.h>

static WORKING_AREA(httpWorkingArea, 1024);

static Thread * httpServerThd = NULL;
static bool killHttpServer = false;
static char rxBuf[1000];

static controlInformation * controlInfo;
static httpInformation httpInfo;
static int serverSocket;

static Mutex * cc3000Mtx;

void clarityCC3000ApiLck(void)
{ 
    if (cc3000Mtx != NULL)      \
    {                           \
        chMtxLock(cc3000Mtx);   \
    }
}

void clarityCC3000ApiUnlck(void)
{
    if (cc3000Mtx != NULL)      \
    {                           \
        chMtxUnlock();          \
    }
}

static msg_t cc3000HttpServer(void * arg)
{
    sockaddr acceptedAddr;
    socklen_t acceptedAddrLen = sizeof(acceptedAddr);
    const char * httpRtn = NULL;
    int16_t rxBytes = 0;
    connectionInformation accepted;
    
    (void)arg;

    memset(&acceptedAddr, 0, sizeof(acceptedAddr));

    while (1)
    {
        PRINT("Server: top of while 1", NULL);

        if (killHttpServer == true)
        {
            break;
        }
 
        memset(&acceptedAddr, 0, sizeof(acceptedAddr));
        memset(rxBuf, 0, sizeof(rxBuf));

        clarityCC3000ApiLck();

        while (true)
        {
            if ((accepted.socket = accept(serverSocket, &acceptedAddr,
                                         &acceptedAddrLen)) < 0)
            {
                if (accepted.socket == -1)
                {
                    PRINT("accept() returned error: %d", accepted.socket);
                }
                else
                {
                    chThdSleep(MS2ST(100));
                }
            }

            else
            {
                break;
            }
        }
        
        if ((rxBytes = recv(accepted.socket, rxBuf, sizeof(rxBuf), 0)) == -1)
        {
            PRINT("recv() returned error.", NULL);
        }
        
        clarityCC3000ApiUnlck();

        if (rxBytes > 0)
        {
            if ((httpRtn = clarityProcess(controlInfo,
                                          &httpInfo, 
                                          &accepted,
                                          rxBuf, rxBytes)) == NULL)
            {
                PRINT("clarityProcess() returned NULL.", NULL);
            }
        }
 
        if (closesocket(accepted.socket) == -1)
        {
            PRINT("closesocket() failed for acceptedSocket.", NULL);
        }

        chThdSleep(MS2ST(500));
    }

    clarityCC3000ApiLck();
    if (closesocket(serverSocket) == -1)
    {
        PRINT("closesocket() failed for serverSocket.", NULL);
    }
    clarityCC3000ApiUnlck();

    return 0;
}

uint32_t clarityHttpServerKill(void)
{
    killHttpServer = true;
    /* TODO ensure server ends */
    return 0;
}

uint32_t clarityHttpServerStart(Mutex * cc3000ApiMtx, controlInformation * control)
{
    uint16_t blockOpt = SOCK_ON;
    sockaddr_in serverAddr = {0};

    cc3000Mtx = cc3000ApiMtx;
    controlInfo = control;

    memset(rxBuf, 0, sizeof(rxBuf));


    /* We need dhcp info here */
    if (cc3000AsyncData.dhcp.present != 1)
    {
        PRINT("No DHCP info present", NULL);
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);
#if 0
    serverAddr.sin_addr.s_addr = *(int *)cc3000AsyncData.dhcp.info.aucIP; 
#endif
    PRINT("ALAN DEBUG DHCP ADDR IS %x", serverAddr.sin_addr.s_addr);


    clarityCC3000ApiLck();

    if ((serverSocket = socket(AF_INET, SOCK_STREAM,
                               IPPROTO_TCP)) == -1)
    {
        PRINT("socket() returned error.", NULL);
        while(1);
    }
    else if (setsockopt(serverSocket, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK,
                        &blockOpt, sizeof(blockOpt)) == -1)
    {
        PRINT("setsockopt() returned error.", NULL);
        while(1);
    }

    else if (bind(serverSocket, (sockaddr *)&serverAddr,
                  sizeof(serverAddr)) == -1)
    {
        serverSocket = -1;
        PRINT("bind() returned error.", NULL);
        while(1);
    }

    else if (listen(serverSocket, 1) != 0)
    {
        serverSocket = -1;
        PRINT("bind() returned error.", NULL);
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

    return 0;
}


