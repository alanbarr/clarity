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


