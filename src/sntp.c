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
    uint32_t timeout = 5000;

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SNTP_PORT);

    if ((rtn = clarityRegisterProcessStarted()) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
        return rtn;
    }

    clarityCC3000ApiLock();
    
    if (gethostbyname(SNTP_SERVER, strlen(SNTP_SERVER),
                      &serverAddr.sin_addr.s_addr) < 0) 
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    else if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    else if (setsockopt(sockfd, SOL_SOCKET, SOCKOPT_RECV_TIMEOUT,
                               &timeout, sizeof(timeout)) != 0)
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    else if ((serverAddr.sin_addr.s_addr = htonl(serverAddr.sin_addr.s_addr))== 0)
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_UNDEFINED;
    }

    else if ((bytes = sendto(sockfd, buf, NTP_PACKET_HEADER_SIZE, 0,
                             (sockaddr *)&serverAddr, sizeof(sockaddr)))
                    != NTP_PACKET_HEADER_SIZE) 
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }
    
    else if ((bytes = recvfrom(sockfd, buf, NTP_PACKET_HEADER_SIZE, 0,
                               (sockaddr *)&serverAddr, &addrlen))
                    != NTP_PACKET_HEADER_SIZE)
    {
        CLAR_PRINT_ERROR();
        rtn = CLARITY_ERROR_CC3000_SOCKET;
    }

    if (sockfd != -1)
    {
        if (closesocket(sockfd) != 0)
        {
            CLAR_PRINT_ERROR();
            rtn = CLARITY_ERROR_CC3000_SOCKET;
        }
    }
    clarityCC3000ApiUnlock();
    rtn = clarityRegisterProcessFinished();
    return rtn;
}

/** @brief Retrieves SNTP time by using the CC3000.
 *  @param[in] buf Buffer used for internal use. Needs to be at 48 bytes.
 *  @param bufSize Size of the buffer.
 *  @param[out] ntpSeconds Remote SNTP server timestamp.
 *  @return Appropriate status from #clarityError. 
 *  @todo API needs improved? Only gets remote end time so could be smarter.*/
clarityError clarityGetSntpTime(char * buf, uint16_t bufSize,
                                uint32_t * ntpSeconds)
{
    clarityError rtn = CLARITY_ERROR_UNDEFINED;

    if (bufSize < NTP_PACKET_HEADER_SIZE)
    {
        CLAR_PRINT_ERROR();
        return CLARITY_ERROR_BUFFER_SIZE;
    }

    if ((rtn = sntpConstructRequest(buf)) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
        return rtn;
    }

    if ((rtn = sntpRequest(buf)) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
        return rtn;
    }

    if ((rtn = sntpParseTransmitTimestamp(buf, ntpSeconds)) != CLARITY_SUCCESS)
    {
        CLAR_PRINT_ERROR();
        return rtn;
    }
    
    return CLARITY_SUCCESS;
}



/** @brief Converts a SNTP time to clarity time structure.
 *  @param[out] clarTD Clarity structure to be populated.
 *  @param sntp sntp time.
 *  @return Appropriate status from #clarityError. */
clarityError clarityTimeFromSntp(clarityTimeDate * clarTD, uint32_t sntp)
{
    if (sntp <= NTP_UNIX_EPOCH_DIFFERENCE)
    {
        return CLARITY_ERROR_RANGE;
    }
    
    return clarityTimeFromUnix(clarTD, sntp - NTP_UNIX_EPOCH_DIFFERENCE);
}

/** @brief Converts a UNIX time to clarity time structure.
 *  @param[out] clarTD Clarity Time and Data structure to be populated.
 *  @param unix UNIX time.
 *  @return Appropriate status from #clarityError. */
clarityError clarityTimeFromUnix(clarityTimeDate * clarTD, time_t unix)
{
    struct tm result;
    time_t time = unix;

    memset(&result, 0, sizeof(result));

    if (gmtime_r(&time, &result) == NULL)
    {
        return CLARITY_ERROR_RANGE;
    }

    if (result.tm_year < 100 || result.tm_year > 199)
    {
        return CLARITY_ERROR_RANGE;
    }

    clarTD->time.hour   = result.tm_hour;
    clarTD->time.minute = result.tm_min;
    clarTD->time.second = result.tm_sec;

    clarTD->date.year  = result.tm_year-100;
    clarTD->date.month = result.tm_mon+1;
    clarTD->date.date  = result.tm_mday;

    if (result.tm_wday == 0)
    {
        clarTD->date.day = 7;
    } 

    else 
    {
        clarTD->date.day = result.tm_wday; 
    }

    return CLARITY_SUCCESS;
}


/** @brief Converts a clarity time structure to UNIX time. 
 *  @param[in] clarTD Clarity time structure to convert.
 *  @param[out] unix unix time.
 *  @return Appropriate status from #clarityError. */
clarityError clarityTimeToUnix(clarityTimeDate * clarTD, time_t * unix)
{
    struct tm unixBD;
    memset(&unixBD, 0, sizeof(unixBD));

    unixBD.tm_hour = clarTD->time.hour;
    unixBD.tm_min  = clarTD->time.minute;
    unixBD.tm_sec  = clarTD->time.second;

    unixBD.tm_year = clarTD->date.year+100;
    unixBD.tm_mon = clarTD->date.month-1;
    unixBD.tm_mday = clarTD->date.date;
    
    unixBD.tm_isdst = -1; /* XXX */

#if 0
    if (clarTD->date.date == 7)
    {
        unixBD->tm_wday = 0;
    } 
    else
    {
        unixBD.tm_wday = clarTD->date.day;
    }
#endif

    if((*unix = mktime(&unixBD)) == -1)
    {
        return CLARITY_ERROR_UNDEFINED;
    }

    return CLARITY_SUCCESS;
}

/** @brief Increments a clarity time strucutre by a number of seconds.
 *  @param timeData Clarity structure to update.
 *  @param seconds Number of seconds to increment. 
 *  @return Appropriate status from #clarityError. */
clarityError clarityTimeIncrement(clarityTimeDate * timeDate, uint32_t seconds)
{
#if 0
    uint32_t temp = seconds;
    uint8_t daysInMonth = getDaysInMonth(timeDate->date.month,
                                         timeDate->date.year)

    if (seconds > DAY_S)
    {
        return 1;
    }

    
    temp += timeDate->time.seconds;
    timeDate->time.seconds = temp % 60;
    temp /= 60;                         /* Now total offset minutes */
    temp += timeDate->time.minutes;
    timeDate->time.minutes = temp % 60;
    temp /= 60;                         /* Now total offset hours */
    temp += timeDate->time.hours;  
    timeDate->time.hours = temp % 24;
    temp /= 24;                         /* Now total offset days */
    timeDate->date.day = (((timeDate->date.day + temp - 1) % 7) + 1);
    temp += timeDate->date.date;

    if (temp > daysInMonth)
    {
        timeDate->date.month++;
        if(timeDate->date.month > 12)
        {
            timeDate->date.years++;
            timeDate->date.month = 1;
        }
        temp-=daysInMonth;
    }

    timeDate->date.date = temp;
#endif
    clarityError rtn;
    time_t unix;

    if ((rtn = clarityTimeToUnix(timeDate, &unix)) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    unix += seconds;

    if ((rtn = clarityTimeFromUnix(timeDate, unix)) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    return CLARITY_SUCCESS;
}
