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
#include "cc3000_chibios_api.h"
#include "clarity_api.h"
#include "clarity_int.h"
#include "socket.h"
#include "wlan.h"

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

#if 0
static WORKING_AREA(connectivityMonThdWorkingArea, 256);
static Thread * connectivityMonThd = NULL;
#endif

static WORKING_AREA(responseMonThdWorkingArea, 256); /* TODO make smaller */
static Thread * responseMonThd = NULL;

static clarityUnresponsiveCallback unresponsiveCb;

#if defined(CLARITY_PRINT_MESSAGES) && CLARITY_PRINT_MESSAGES == TRUE
clarityPrintCb clarityPrint;
#endif

/* must already be powered on.
 * MGMT LOCK/UNLOCK NEEDS CALLED EXTERNALLY */
static clarityError connectToWifi_mtxext(void)
{
    int32_t wlanRtn;
    uint8_t loopIterations = 10;
    uint8_t presentCount = 0;

    clarityCC3000ApiLock();

    memset((void*)&cc3000AsyncData, 0, sizeof(cc3000AsyncData));

    CLAR_PRINT_LINE("about to call wlan_connect()");
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
                CLAR_PRINT_LINE_ARGS("Connected: %d DHCP Present: %d. Breaking!",
                                     cc3000AsyncData.connected,
                                     cc3000AsyncData.dhcp.present);
                break;
            }

            chThdSleep(MS2ST(500));
        }
    }
    clarityCC3000ApiUnlock();

    if (cc3000AsyncData.connected == true &&
        cc3000AsyncData.dhcp.present == true)
    {
        CLAR_PRINT_LINE("Connected!");
    }

    if (wlanRtn == 0)
    {
        return CLARITY_SUCCESS;
    }
    else
    {
        CLAR_PRINT_ERROR();
        return CLARITY_ERROR_CC3000_WLAN;
    }
}

static clarityError clarityMgmtCheckNeedForConnectivty(void)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    clarityMgmtMtxLock();

    if (cc3000AsyncData.connected == false &&
        mgmtData.activeProcesses != 0)
    {
        CLAR_PRINT_ERROR();
#if 0
        long statusRtn; /*XXX*/
        if (statusRtn = wlan_ioctl_statusget()) /* XXX */
        {}
        CLAR_PRINT_ERROR();
        CLAR_PRINT_LINE_ARGS("status get: %d", statusRtn);
#endif
        rtn = connectToWifi_mtxext();
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
            wlan_disconnect();
            wlan_stop();       
            clarityCC3000ApiUnlock();

            mgmtData.active = false;
            /* TODO what can we do about asyncdata shutdown ok??? */
        }
    }
    clarityMgmtMtxUnlock();

    return CLARITY_SUCCESS;
}

/* mgmt mutex MUST be locked by caller */
static clarityError clarityMgmtAttemptActivate_mtxext(void)
{
    clarityError rtn = CLARITY_SUCCESS;
    /* TODO.
     * EEPROM functions should not be used this frequently. A better 
     * way of implementing this in software is needed OR a shell to allow
     * the user to perform a one time setup */
#if 0
    uint32_t ip = 0;
    uint32_t subnet = 0;
    uint32_t gateway = 0;
    uint32_t dns = 0;
#endif

    if (mgmtData.active == true)
    {
        return CLARITY_SUCCESS;
    }


#if 0
    if (mgmtData.ap->deviceIp.isStatic == true)
    {
        ip = htonl(mgmtData.ap->deviceIp.ip);
        subnet = htonl(mgmtData.ap->deviceIp.subnet);
        gateway = htonl(mgmtData.ap->deviceIp.gateway);
        dns = htonl(mgmtData.ap->deviceIp.dns);
    }
#endif

    clarityCC3000ApiLock();
    wlan_start(0);       
    mgmtData.active = true;

#if 0
    if (netapp_dhcp(&ip, &subnet, &gateway, &dns) != 0)
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_CC3000_NETAPP;
    }

    wlan_disconnect();
    wlan_stop();
    chThdSleep(MS2ST(200));
    wlan_start(0);
#endif
    clarityCC3000ApiUnlock();

    if ((rtn = connectToWifi_mtxext()) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
    }

    if (rtn != CLARITY_SUCCESS)
    {
        clarityCC3000ApiLock();
        wlan_disconnect();
        wlan_stop();       
        clarityCC3000ApiUnlock();

        mgmtData.active = false;

    }

    return rtn;
}

/** @brief Indicates to clarity that a process is using the CC3000. 
 *  @brief This will active the CC3000 module and connect to a network (unless 
 *         already connected).
 *  @return Appropriate status from #clarityError. */
clarityError clarityRegisterProcessStarted(void)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    clarityMgmtMtxLock();
    mgmtData.activeProcesses++;

    if ((rtn = clarityMgmtAttemptActivate_mtxext()) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
        mgmtData.activeProcesses--;
    }

    clarityMgmtMtxUnlock();
    return rtn;
}


/** @brief Indicates to clarity that a process has finished using the CC3000. 
 *  @brief This will deactive the CC3000 module if no other process is using 
 *         the cc3000.
 *  @return Appropriate status from #clarityError. */
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

#if 0
static msg_t clarityMgmtConnectivityMonitoringThd(void *arg)
{
    (void)arg;
    
    #if CH_USE_REGISTRY == TRUE
    chRegSetThreadName(__FUNCTION__);
    #endif

    while (chThdShouldTerminate() == FALSE)
    {
        chThdSleep(MS2ST(500));

#if 0
        /* Check for disconnect */
        clarityMgmtCheckNeedForConnectivty();
#endif

# if 0
        /* Check to see if we can power down */
        clarityMgmtAttemptPowerDown();
#endif

    }

    return CLARITY_SUCCESS;
}
#endif


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
    chMtxInit(&mgmtData.mutex);

    clarityMgmtMtxLock();
    mgmtData.ap = apInfo;
    clarityMgmtMtxUnlock();

#if 0
    connectivityMonThd = chThdCreateStatic(connectivityMonThdWorkingArea,
                                sizeof(connectivityMonThdWorkingArea),
                                NORMALPRIO + 1,
                                clarityMgmtConnectivityMonitoringThd,          
                                NULL);         
#endif
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

#if 0
    clarityMgmtMtxLock();
    chThdTerminate(connectivityMonThd);
    clarityMgmtMtxUnlock();
    chThdWait(connectivityMonThd);
#endif

    if (responseMonThd != NULL)
    {
        chThdTerminate(responseMonThd);
        chThdWait(responseMonThd);
    }

    return CLARITY_SUCCESS;
}

/** @brief Locks the CC3000 API mutex. */
void clarityCC3000ApiLock(void)
{ 
    if (cc3000Mtx != NULL)
    {
        chMtxLock(cc3000Mtx);
    }
}

/** @brief Unlocks the CC3000 API mutex. */
void clarityCC3000ApiUnlock(void)
{
    if (cc3000Mtx != NULL)
    {
        chMtxUnlock();
    }
}


/** @brief Responsible for initialising the module.
 *  @param cc3000ApiMtx Mutex to protect CC3000 Host Driver API Calls. Needs
 *                      to be externally initialised.
 *  @param cb Callback that is called when clarity determines the CC3000
 *             has become unresponsive. 
 *  @param accessPointConnection Information about the access point to use.
 *  @param printCb Callback to a print function. Only called if 
 *                 #CLARITY_PRINT_MESSAGES is TRUE.
 *  @return Appropriate status from #clarityError. */
clarityError clarityInit(Mutex * cc3000ApiMtx,
                         clarityUnresponsiveCallback cb,
                         clarityAccessPointInformation * accessPointConnection,
                         clarityPrintCb printCb)
{
    cc3000Mtx = cc3000ApiMtx;

#if CLARITY_PRINT_MESSAGES == TRUE
    clarityPrint = printCb;
#endif

    return clarityMgmtInit(accessPointConnection, cb);
}

/** @details Responsive for shutting down the module.
 *  @return Appropriate status from #clarityError. */
clarityError clarityShutdown(void)
{
   return clarityMgmtShutdown();
}


