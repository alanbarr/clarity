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
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "clarity_api.h"
#include "clarity_int.h"
#include "socket.h"

#define SNTP_PORT                       123
#define SNTP_SERVER                     "pool.ntp.org"
#define SNTP_VERSION_NUMBER             4

#define NTP_PACKET_HEADER_SIZE          48

#define NTP_LEAP_INDICATOR_SHIFT        (6)
#define NTP_LEAP_INDICATOR_MASK         (0x3<<NTP_LEAP_INDICATOR_SHIFT)
#define NTP_VERSION_NUMBER_SHIFT        (3)
#define NTP_VERSION_NUMBER_MASK         (0x7<<NTP_VERSION_NUMBER_SHIFT)
#define NTP_MODE_MASK                   (0x7)

#define NTP_MODE_CLIENT                 3
#define NTP_MODE_SERVER                 4

#define NTP_SECONDS_INDEX               0
#define NTP_SECONDS_FRACTION_INDEX      1

#define NTP_UNIX_EPOCH_DIFFERENCE       2208988800U

#define compile_time_assert(X)                                              \
    extern int (*compile_assert(void))[sizeof(struct {                      \
                unsigned int compile_assert_failed : (X) ? 1 : -1; })]

typedef struct {
    uint8_t lvm;
    uint8_t stratum;
    uint8_t poll;
    uint8_t percision;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t referenceIndicator;
    uint32_t referenceTimestamp[2];
    uint32_t originateTimestamp[2];
    uint32_t receiveTimestamp[2];
    uint32_t transmitTimestamp[2];
#if 0 /* Optional */
    uint32_t keyIdentifier;
    uint32_t messageDigest[4];
#endif
} ntpPacketHeader;

compile_time_assert(sizeof(ntpPacketHeader) == NTP_PACKET_HEADER_SIZE);

static clarityError sntpParseTransmitTimestamp(char * buf, uint32_t * ntpTime)
{
    ntpPacketHeader * ph = (ntpPacketHeader*)buf;
    
    if ((ph->lvm & NTP_MODE_MASK) != NTP_MODE_SERVER)
    {
        return CLARITY_ERROR_REMOTE_RESPONSE;       
    }

    if (ph->stratum == 0)
    {
        /* TODO we are in their bad books - presumably we can't trust the 
         * time ? */
        return CLARITY_ERROR_REMOTE_RESPONSE;       
    }

    *ntpTime = ntohl(ph->transmitTimestamp[NTP_SECONDS_INDEX]);
    return CLARITY_SUCCESS;
}

static clarityError sntpConstructRequest(char * buf)
{
    ntpPacketHeader * ph = (ntpPacketHeader*)buf;

    memset(ph, 0, sizeof(ntpPacketHeader));

    ph->lvm = NTP_MODE_CLIENT |
              SNTP_VERSION_NUMBER << NTP_VERSION_NUMBER_SHIFT;

    return CLARITY_SUCCESS;
}


static clarityError sntpRequest(char * buf)
{
    int32_t sockfd = -1;
    sockaddr_in serverAddr;
    int16_t bytes = -1;
    socklen_t addrlen = sizeof(serverAddr);
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SNTP_PORT);

    if ((rtn = clarityMgmtRegisterProcessStarted()) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    clarityCC3000ApiLck();
    
    if (gethostbyname(SNTP_SERVER, strlen(SNTP_SERVER),
                           &serverAddr.sin_addr.s_addr) < 0) 
    {
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    else if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    else if ((serverAddr.sin_addr.s_addr = htonl(serverAddr.sin_addr.s_addr))== 0)
    {
        rtn = CLARITY_ERROR_UNDEFINED;
    }

    else if ((bytes = sendto(sockfd, buf, NTP_PACKET_HEADER_SIZE, 0,
                             (sockaddr *)&serverAddr, sizeof(sockaddr)))
                    != NTP_PACKET_HEADER_SIZE) 
    {
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }
    
    else if ((bytes = recvfrom(sockfd, buf, NTP_PACKET_HEADER_SIZE, 0,
                               (sockaddr *)&serverAddr, &addrlen))
                    != NTP_PACKET_HEADER_SIZE)
    {
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    if (sockfd != -1)
    {
        if (closesocket(sockfd) != 0)
        {
            rtn = CLARITY_ERROR_CC3000_SOCKET;
        }
    }
    clarityCC3000ApiUnlck();
    rtn = clarityMgmtRegisterProcessFinished();
    return rtn;
}


clarityError clarityGetSntpTime(char * buf, uint16_t bufSize,
                                uint32_t * ntpSeconds)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    if (bufSize < NTP_PACKET_HEADER_SIZE)
    {
        return CLARITY_ERROR_BUFFER_SIZE;
    }

    if ((rtn = sntpConstructRequest(buf)) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    if ((rtn = sntpRequest(buf)) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    if ((rtn = sntpParseTransmitTimestamp(buf, ntpSeconds)) != CLARITY_SUCCESS)
    {
        return rtn;
    }
    
    return CLARITY_SUCCESS;
}


clarityError claritySntpTimeBrokenDown(clarityTimeDate * brokenDown,
                                       uint32_t sntpSeconds)
{
    struct tm result;
    time_t time = 0;

    memset(&result, 0, sizeof(result));

    if (sntpSeconds <= NTP_UNIX_EPOCH_DIFFERENCE)
    {
        return CLARITY_ERROR_RANGE;
    }
    
    time = sntpSeconds - NTP_UNIX_EPOCH_DIFFERENCE;

    if (gmtime_r(&time, &result) == NULL)
    {
        return CLARITY_ERROR_RANGE;
    }

    if (result.tm_year < 100 || result.tm_year > 199)
    {
        return CLARITY_ERROR_RANGE;
    }

    brokenDown->time.hour   = result.tm_hour;
    brokenDown->time.minute = result.tm_min;
    brokenDown->time.second = result.tm_sec;

    brokenDown->date.year  = result.tm_year-100;
    brokenDown->date.month = result.tm_mon+1;
    brokenDown->date.date  = result.tm_mday;
    brokenDown->date.day   = result.tm_wday; 

    if (brokenDown->date.day == 0)
    {
        brokenDown->date.day = 7;
    } 

    return CLARITY_SUCCESS;
}

