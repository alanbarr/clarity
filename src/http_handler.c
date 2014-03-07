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


#include "clarity_api.h"
#include <strings.h>


/* TODO use to cut short processing */
bool httpHandlerCheckResouceMethod(httpInformation * info)
{
    (void)info;
    return true;
}

static bool getMethod(resourceInformation * resInfo, methodType method, 
                      methodInformation ** methodInfo)
{
    uint8_t methIndex;

    for (methIndex=0; methIndex<MAX_METHODS; methIndex++)
    {
        if (resInfo->methods[methIndex].type == method)
        {
            *methodInfo=&resInfo->methods[methIndex];
            return true;
        }
    }

    return false;
}

static bool getResource(controlInformation * con, buf * resBuf,
                        resourceInformation ** resInfo)
{
    uint8_t resIndex;
    for (resIndex=0; resIndex<MAX_RESOURCES; resIndex++)
    {
        if(strncasecmp(resBuf->data, con->resources[resIndex].name,
                       resBuf->size) == 0)
        {
            *resInfo=&con->resources[resIndex];
            return true;
        }
    }
    return false;
}


static bool httpHandle(httpInformation * info,
                       controlInformation * control, void * user)
{
    resourceInformation * resInfo;
    methodInformation * methInfo;

    if (true != getResource(control, &info->resource, &resInfo))
    {

    }

    else if (true != getMethod(resInfo, info->type, &methInfo))
    {

    }

    else
    {
        if (methInfo->callback != NULL)
        {
            methInfo->callback(info, user);
        }

        return true;
    }

    return false;
}


const char * clarityProcess(controlInformation * control,
                            httpInformation * http,
                            connectionInformation * connection,
                            const char * data, uint16_t size)
{
    const char * rtnd = NULL;
    
    if (NULL == (rtnd = httpParse(http, data, size)))
    {
        return NULL;
    }

    else if (httpHandle(http, control, connection) != true)
    {
        return NULL;
    }

    else 
    {
        return rtnd;
    }

    return NULL;
}


