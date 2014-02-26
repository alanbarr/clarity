#include "http_handler.h"
#include <strings.h>


void httpRegisterControl(controlInformation * userControl)
{
    /* TODO delete me */
}

/* TODO use to cut short processing */
bool httpHandlerCheckResouceMethod(httpInformation * info)
{
    return true;
}

static bool getMethod(resourceInformation * resInfo, methodType method, 
                      methodInformation ** methodInfo)
{
    uint8_t methIndex;

    if ((resInfo->methodsMask & METHOD_TO_MASK(method)) == 0) /* TODO consider getting rid of mask */
    {
        return false;
    }

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
                            httpInformation * info,
                            const char * data, uint16_t size, void * user)
{
    const char * rtnd = NULL;
    
    if (NULL == (rtnd = httpParse(info, data, size)))
    {
        return NULL;
    }

    else if (httpHandle(info, control, user) != true)
    {
        return NULL;
    }

    else 
    {
        return rtnd;
    }

    return NULL;
}


