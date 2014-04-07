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
#include "cc3000_chibios_api.h"

#define CC3000_MUTEX_LOCKUP_S       60
#define CC3000_MUTEX_POLL_TIME_MS   500
#define CC3000_MUTEX_POLL_COUNT     (CC3000_MUTEX_LOCKUP_S * 1000/    \
                                     CC3000_MUTEX_POLL_TIME_MS)

#define clarityMgmtMtxLock()        chMtxLock(&mgmtData.mutex)
#define clarityMgmtMtxUnlock()      chMtxUnlock()

typedef struct {
    Mutex mutex;
    bool active;                /* CC3000 Power */
    uint16_t activeProcesses;   /* Number of running user processes */
    clarityAccessPointInformation * ap;
} clarityMgmtData;

static clarityMgmtData mgmtData;

static Mutex * cc3000Mtx;

static WORKING_AREA(connectivityMonThdWorkingArea, 256);
static Thread * connectivityMonThd = NULL;

static WORKING_AREA(responseMonThdWorkingArea, 256);
static Thread * responseMonThd = NULL;

static clarityUnresponsiveCallback unresponsiveCb;

#if defined(CLARITY_PRINT_MESSAGES) && CLARITY_PRINT_MESSAGES == TRUE
clarityPrintCb clarityPrint;
#endif

/* must already be powered on.
 * MGMT LOCK/UNLOCK NEEDS CALLED EXTERNALLY */
static clarityError connectToWifi(void)
{
    int32_t wlanRtn;
    uint8_t loopIterations = 10;
    uint8_t presentCount = 0;

    clarityCC3000ApiLock();

    memset((void*)&cc3000AsyncData, 0, sizeof(cc3000AsyncData));

    CLAR_PRINT_LINE("about to  call wlan_connect()");
    if (mgmtData.ap->secType == WLAN_SEC_UNSEC)
    {
        wlanRtn = wlan_connect(mgmtData.ap->secType,
                               mgmtData.ap->name,
                               strnlen(mgmtData.ap->name, CLARITY_MAX_AP_STR_LEN),
                               NULL, NULL, 0);
    }
    else
    {
        wlanRtn = wlan_connect(mgmtData.ap->secType,
                               mgmtData.ap->name,
                               strnlen(mgmtData.ap->name, CLARITY_MAX_AP_STR_LEN),
                               NULL,
                               (unsigned char*)mgmtData.ap->password,
                               strnlen(mgmtData.ap->password, CLARITY_MAX_AP_STR_LEN));
    }
    CLAR_PRINT_LINE("called wlan_connect()");

    if (wlanRtn != 0)
    {
        CLAR_PRINT_LINE("wlan_connect() returned non zero.");
    }

    else
    {
        while (presentCount != 3)
        {
            if (cc3000AsyncData.connected != true || 
                cc3000AsyncData.dhcp.present != true)
            {
                presentCount = 0;
            }

            else 
            {
                presentCount++;
            }

            if (loopIterations-- == 0)
            {
                wlanRtn = 1;
                CLAR_PRINT_LINE("Breaking!");
                break;
            }

            chThdSleep(MS2ST(500));
        }
    }
    clarityCC3000ApiUnlock();

    if (wlanRtn == 0)
    {
        return CLARITY_SUCCESS;
    }
    else
    {
        return CLARITY_ERROR_CC3000_WLAN;
    }
}

static clarityError clarityMgmtCheckNeedForConnectivty(void)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;
    long statusRtn; /*XXX*/

    clarityMgmtMtxLock();

    if (cc3000AsyncData.connected == false &&
        mgmtData.activeProcesses != 0)
    {
        CLAR_PRINT_ERROR();
        if (statusRtn = wlan_ioctl_statusget()) /* XXX */
        {}
        CLAR_PRINT_ERROR();
        CLAR_PRINT_LINE_ARGS("status get: %d", statusRtn);
        rtn = connectToWifi();
    }

    clarityMgmtMtxUnlock();

    return rtn;
}

static clarityError clarityMgmtAttemptPowerDown(void)
{
    clarityMgmtMtxLock();
    if (mgmtData.active == true)
    {
        if (mgmtData.activeProcesses == 0)
        {
            clarityCC3000ApiLock();
            wlan_stop();       
            clarityCC3000ApiUnlock();

            mgmtData.active = false;
            clarityMgmtMtxUnlock();
            /* TODO what can we do about asyncdata shutdown ok??? */
        }
    }
    clarityMgmtMtxUnlock();

    return CLARITY_SUCCESS;
}

/* mgmt mutex MUST be locked by caller */
static clarityError clarityMgmtAttemptActivate(void)
{
    clarityError rtn = CLARITY_SUCCESS;

    if (mgmtData.active == true)
    {
        return CLARITY_SUCCESS;
    }

    if (mgmtData.ap->deviceIp.isStatic == false)
    {
        mgmtData.ap->deviceIp.ip = 0;
        mgmtData.ap->deviceIp.subnet = 0;
        mgmtData.ap->deviceIp.gateway = 0;
        mgmtData.ap->deviceIp.dns = 0;
    }

    clarityCC3000ApiLock();
    wlan_start(0);       
    mgmtData.active = true;

    if (netapp_dhcp(&(mgmtData.ap->deviceIp.ip),
                    &(mgmtData.ap->deviceIp.subnet),
                    &(mgmtData.ap->deviceIp.gateway),
                    &(mgmtData.ap->deviceIp.dns)) != 0)
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_CC3000_NETAPP;
    }
    clarityCC3000ApiUnlock();

    clarityMgmtMtxLock();
    if ((rtn = connectToWifi()) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
    }
    clarityMgmtMtxUnlock();

    if (rtn != CLARITY_SUCCESS)
    {
        clarityCC3000ApiLock();
        wlan_stop();       
        clarityCC3000ApiUnlock();

        clarityMgmtMtxLock();
        mgmtData.active = false;
        clarityMgmtMtxUnlock();

    }

    return rtn;
}

clarityError clarityRegisterProcessStarted(void)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    clarityMgmtMtxLock();
    mgmtData.activeProcesses++;

    if ((rtn = clarityMgmtAttemptActivate()) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
        mgmtData.activeProcesses--;
    }

    clarityMgmtMtxUnlock();
    return rtn;
}

clarityError clarityRegisterProcessFinished(void)
{
    clarityMgmtMtxLock();
    if (mgmtData.activeProcesses == 0)
    {
        clarityMgmtMtxUnlock();
        return CLARITY_ERROR_STATE;
    }
    mgmtData.activeProcesses--;
    clarityMgmtMtxUnlock();
    clarityMgmtAttemptPowerDown();
    return CLARITY_SUCCESS;
}

static msg_t clarityMgmtConnectivityMonitoringThd(void *arg)
{
    (void)arg;
    
    #if CH_USE_REGISTRY == TRUE
    chRegSetThreadName(__FUNCTION__);
    #endif

    while (chThdShouldTerminate() == FALSE)
    {
        chThdSleep(MS2ST(500));

        /* Check for disconnect */
        clarityMgmtCheckNeedForConnectivty();

# if 0
        /* Check to see if we can power down */
        clarityMgmtAttemptPowerDown();
#endif

    }

    return CLARITY_SUCCESS;
}


static msg_t clarityMgmtResponseMonitoringThd(void *arg)
{
    uint32_t attempts = 0;

    (void)arg;
    
    #if CH_USE_REGISTRY == TRUE
    chRegSetThreadName(__FUNCTION__);
    #endif

    while (chThdShouldTerminate() == FALSE)
    {
        if (chMtxTryLock(cc3000Mtx) == TRUE)
        {
            chMtxUnlock();
            attempts = 0;
        }
        else 
        {
            attempts++;
            if (attempts == CC3000_MUTEX_POLL_COUNT)
            {
                unresponsiveCb();
            }
        }

        chThdSleep(MS2ST(CC3000_MUTEX_POLL_TIME_MS));
    }
    return CLARITY_SUCCESS;
}

static clarityError clarityMgmtInit(clarityAccessPointInformation * apInfo,
                                    clarityUnresponsiveCallback cb)
{

    memset(&mgmtData, 0, sizeof(mgmtData));

    clarityMgmtMtxLock();
    chMtxInit(&mgmtData.mutex);
    mgmtData.ap = apInfo;
    clarityMgmtMtxUnlock();

    connectivityMonThd = chThdCreateStatic(connectivityMonThdWorkingArea,
                                sizeof(connectivityMonThdWorkingArea),
                                NORMALPRIO + 1,
                                clarityMgmtConnectivityMonitoringThd,          
                                NULL);         
    if (cb != NULL)
    {
        unresponsiveCb = cb;
        responseMonThd = chThdCreateStatic(responseMonThdWorkingArea,
                                sizeof(responseMonThdWorkingArea),
                                HIGHPRIO-1,
                                clarityMgmtResponseMonitoringThd,          
                                NULL);       
    }
    return CLARITY_SUCCESS;
}


static clarityError clarityMgmtShutdown(void)
{
    if (mgmtData.activeProcesses != 0)
    {
        return CLARITY_ERROR_STATE;
    }

    clarityHttpServerStop();

    clarityMgmtMtxLock();
    chThdTerminate(connectivityMonThd);
    clarityMgmtMtxUnlock();
    chThdWait(connectivityMonThd);

    if (responseMonThd != NULL)
    {
        chThdTerminate(responseMonThd);
        chThdWait(responseMonThd);
    }

    return CLARITY_SUCCESS;
}

void clarityCC3000ApiLock(void)
{ 
    if (cc3000Mtx != NULL)
    {
        chMtxLock(cc3000Mtx);
    }
}

void clarityCC3000ApiUnlock(void)
{
    if (cc3000Mtx != NULL)
    {
        chMtxUnlock();
    }
}


#if CLARITY_PRINT_MESSAGES == FALSE
clarityError clarityInit(Mutex * cc3000ApiMtx,
                         clarityUnresponsiveCallback cb,
                         clarityAccessPointInformation * accessPointConnection)
{
    cc3000Mtx = cc3000ApiMtx;
    return clarityMgmtInit(accessPointConnection, cb);
}
#else

clarityError clarityInit(Mutex * cc3000ApiMtx,
                         clarityUnresponsiveCallback cb,
                         clarityAccessPointInformation * accessPointConnection,
                         clarityPrintCb printCb)
{
    cc3000Mtx = cc3000ApiMtx;
    clarityPrint = printCb;
    return clarityMgmtInit(accessPointConnection, cb);
}
#endif

clarityError clarityShutdown(void)
{
   return clarityMgmtShutdown();
}


