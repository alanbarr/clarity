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

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <netdb.h>
#include <stdio.h>
typedef int32_t clarityError;
#define CLARITY_SUCCESS 0
#define CLARITY_ERROR_BUFFER_SIZE !CLARITY_SUCCESS
#define CLARITY_ERROR_REMOTE_RESPONSE CLARITY_ERROR_BUFFER_SIZE

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


clarityError sntpRequest(char * buf)
{
    int sockfd;
    struct sockaddr_in their_addr;
    int addr;
    struct hostent *he;
    int numbytes;
    int addrlen = sizeof(struct sockaddr);

    if ((he=gethostbyname(SNTP_SERVER)) == NULL) 
    {
        printf("%s %d exit!", __FILE__, __LINE__);
        exit(1);
    }
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        perror("socket");
        exit(1);
    }
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(SNTP_PORT);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), 0, 8);

    if ((numbytes=sendto(sockfd, buf, NTP_PACKET_HEADER_SIZE, 0,
                         (struct sockaddr *)&their_addr,
                         sizeof(struct sockaddr))) == -1) 
    {
        perror("sendto");
        exit(1);
    }
    
    printf("sent %d bytes\n", numbytes);

    if ((numbytes = recvfrom(sockfd, buf, NTP_PACKET_HEADER_SIZE, 0,
                            (struct sockaddr *)&their_addr,
                            &addrlen)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }

    printf("rx %d bytess\n", numbytes);
    close(sockfd);
    return CLARITY_SUCCESS;
}


clarityError clarityGetSntpTime(char * buf, uint16_t bufSize, uint32_t * ntpTime)
{
    clarityError rtn;

    if (bufSize < NTP_PACKET_HEADER_SIZE)
    {
        return CLARITY_ERROR_BUFFER_SIZE;
    }

    if ((rtn = sntpConstructRequest(buf))
             != CLARITY_SUCCESS)
    {
        return rtn;
    }

    if ((rtn = sntpRequest(buf)) != CLARITY_SUCCESS)
    {
        return rtn;
    }

    if ((rtn = sntpParseTransmitTimestamp(buf, ntpTime)) != CLARITY_SUCCESS)
    {
        return rtn;
    }
    
    return CLARITY_SUCCESS;
}

int main(void)
{
    char buf[NTP_PACKET_HEADER_SIZE];
    uint32_t t = 0;
    printf("rtn: %d\n", clarityGetSntpTime(buf, sizeof(buf),&t));
    printf("time: %u %x\n", t, t);
    return 0;
}
