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

#ifndef __CLARITY_INT_H__
#define __CLARITY_INT_H__

#include "clarity_api.h"

clarityError clarityMgmtRegisterProcessStarted(void);
clarityError clarityMgmtRegisterProcessFinished(void);
int32_t httpHandle(clarityHttpRequestInformation * info,
                   clarityHttpServerInformation * control,
                   void * user);
const char * httpRequestProcess(clarityHttpServerInformation * control,
                                clarityHttpRequestInformation * http,
                                clarityConnectionInformation * connection,
                                const char * data, uint16_t size);
const char * httpParseRequest(clarityHttpRequestInformation * info,
                              const char * data, const uint16_t size);
int32_t httpBuildRequestTextPlain(const clarityHttpRequestInformation * http,
                                  char * txBuf,
                                  uint16_t txBufSize);

const char * httpParseResponse(clarityHttpResponseInformation * info,
                               const char * data,
                               uint16_t size);
#endif /* __CLARITY_INT_H__ */
