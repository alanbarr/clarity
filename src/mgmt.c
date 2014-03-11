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
#include "cc3000_chibios_api.h"

static Mutex mgmtMutex;
 
typedef struct {
    bool active;                /* CC3000 Power */
    uint16_t activeProcesses;   /* Number of running user processes */
    accessPointInformation * ap;
} clarityMgmtData;

clarityMgmtData mgmtData;

static WORKING_AREA(clarityMgmtThdWorkingArea, 256);

#if 0
static bool killMgmtThd;
#endif
static Thread * mgmtThd = NULL;

/* must already be powered on */
static clarityError connectToWifi(void)
{
    clarityCC3000ApiLck();
    int32_t wlanRtn;

    if (mgmtData.ap->secType == WLAN_SEC_UNSEC)
    {
        wlanRtn = wlan_connect(mgmtData.ap->secType,
                               mgmtData.ap->name,
                               strnlen(mgmtData.ap->name, MAX_AP_STR_LEN),
                               NULL, NULL, 0);
    }
    else
    {
        wlanRtn = wlan_connect(mgmtData.ap->secType,
                               mgmtData.ap->name,
                               strnlen(mgmtData.ap->name, MAX_AP_STR_LEN),
                               NULL,
                               (unsigned char*)mgmtData.ap->password,
                               strnlen(mgmtData.ap->password, MAX_AP_STR_LEN));
    }

    while(cc3000AsyncData.dhcp.present != true)
    {
        chThdSleep(MS2ST(500));
        /* TODO some kind of DHCP timeout */
        if (cc3000AsyncData.connected == false)
        {
            wlanRtn = 1; /* TODO assuming we have got the cb at this point */
            break;
        }
    }

    clarityCC3000ApiUnlck();

    if (wlanRtn == 0)
    {
        return CLARITY_SUCCESS;
    }
    else
    {
        return CLARITY_ERROR;
    }
}

static clarityError clarityMgmtCheckNeedForConnectivty(void)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;
    chMtxLock(&mgmtMutex);
    if (cc3000AsyncData.connected == false &&
        mgmtData.activeProcesses != 0)
    {
         rtn = connectToWifi();
    }
    chMtxUnlock();

    return rtn;
}

static clarityError clarityMgmtAttemptPowerDown(void)
{
    chMtxLock(&mgmtMutex);
    if (mgmtData.active == true)
    {
        if (mgmtData.activeProcesses == 0)
        {
            clarityCC3000ApiLck();
            wlan_stop();       
            clarityCC3000ApiUnlck();
            mgmtData.active = false;
            /* TODO what can we do about asyncdata shutdown ok??? */
        }
    }
    chMtxUnlock();

    return CLARITY_SUCCESS;
}

static clarityError clarityMgmtAttemptActivate(void)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    clarityCC3000ApiLck();
    wlan_start(0);       
    clarityCC3000ApiUnlck();

    chMtxLock(&mgmtMutex);
    mgmtData.active = true;
    chMtxUnlock();

    if ((rtn = connectToWifi()) != CLARITY_SUCCESS)
    {
        chMtxLock(&mgmtMutex);
        mgmtData.active = false;
        chMtxUnlock();
 
        clarityCC3000ApiLck();
        wlan_stop();       
        clarityCC3000ApiUnlck();
    }
    return rtn;
}

clarityError clarityMgmtRegisterProcessStarted(void)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    chMtxLock(&mgmtMutex);
    mgmtData.activeProcesses++;
    chMtxUnlock();

    if ((rtn = clarityMgmtAttemptActivate()) != CLARITY_SUCCESS)
    {
        chMtxLock(&mgmtMutex);
        mgmtData.activeProcesses--;
        chMtxUnlock();
    }
    return rtn;
}

clarityError clarityMgmtRegisterProcessFinished(void)
{
    chMtxLock(&mgmtMutex);
    if (mgmtData.activeProcesses == 0)
    {
        return 1;
    }
    mgmtData.activeProcesses--;
    chMtxUnlock();
    return 0;
}


static msg_t clarityMgmtThd(void *arg)
{
    (void)arg;

    while (true)
    {
#if 0
        chMtxLock(&mgmtMutex);
        if (killMgmtThd == true)
        {
            chMtxUnlock();
            break;
        }
        chMtxUnlock();
#endif

        /* Check for disconnect */
        clarityMgmtCheckNeedForConnectivty();

        /* Check to see if we can power down */
        clarityMgmtAttemptPowerDown();

        chThdSleep(MS2ST(500));
    }

    return 0;
}
int32_t clarityMgmtInit(accessPointInformation * accessPointConnection)
{
    chMtxInit(&mgmtMutex);
    memset(&mgmtData, 0, sizeof(mgmtData));

    chMtxLock(&mgmtMutex);
#if 0
    killMgmtThd = false;
#endif
    chMtxUnlock();

    mgmtData.ap = accessPointConnection;

    mgmtThd = chThdCreateStatic(clarityMgmtThdWorkingArea,
                                sizeof(clarityMgmtThdWorkingArea),
                                NORMALPRIO + 10,   /* TODO */
                                clarityMgmtThd,          
                                NULL);         
    return 0;
}





