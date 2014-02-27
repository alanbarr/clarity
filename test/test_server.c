#include <string.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include "clarity_api.h"

#define ADDR_STR "localhost"
#define PORT_STR "56123"

#define TEST_PRINT(S) \
    printf("TEST (%s:%d) " S "\n", __FILE__, __LINE__)

#define TEST_PRINT_ARG(S, ...) \
    printf("TEST (%s:%d) " S "\n", __FILE__, __LINE__, __VA_ARGS__)

static uint32_t generalPut(const char * body, const char * data, 
                           const uint16_t size, void * user)
{   
    TEST_PRINT_ARG("In %s.", __FUNCTION__);
    return 0;
}   

uint32_t generalGet(const char * body, const char * data,
                    const uint16_t size, void * user)
{
    TEST_PRINT_ARG("In %s.", __FUNCTION__);
    return 0;
}   

uint32_t generalPost(const char * body, const char * data,
                     const uint16_t size, void * user)
{
    TEST_PRINT_ARG("In %s.", __FUNCTION__);
    return 0;
}   


static void setupControl(controlInformation * control)
{
    control->deviceName = "Test_Control";

    control->resources[0].name = "/";
    control->resources[0].methodsMask = GET_MASK;
    control->resources[0].methods[0].type = GET;
    control->resources[0].methods[0].callback = generalGet;
 
    control->resources[1].name = "/path/script.cgi";
    control->resources[1].methodsMask = POST_MASK;
    control->resources[1].methods[0].type = POST;
    control->resources[1].methods[0].callback = generalPost;
 
    control->resources[2].name = "/robots.txt";
    control->resources[2].methodsMask = GET_MASK | PUT_MASK;
    control->resources[2].methods[0].type = GET;
    control->resources[2].methods[0].callback = generalGet;
    control->resources[2].methods[1].type = PUT;
    control->resources[2].methods[1].callback = generalPut;
}

static void runServer(controlInformation * control)
{
    int sockfd;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0 , sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;     

    if (0 != getaddrinfo(ADDR_STR, PORT_STR, &hints, &servinfo))
    {
        TEST_PRINT("getAddrInfo failed");
        TEST_PRINT_ARG("errno: %d", errno);
        TEST_PRINT_ARG("errno: %s", strerror(errno));
    }

    else if (-1 == (sockfd = socket(servinfo->ai_family,
                                    servinfo->ai_socktype,
                                    servinfo->ai_protocol)))
    {
        TEST_PRINT("Socket failed");
    }

    else if (-1 == (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)))
    { 
        TEST_PRINT("bind failed");
    }

    else if (-1 == (listen(sockfd, 1)))
    { 
        TEST_PRINT("bind failed");
    }

    while(1)
    {
        struct sockaddr addrthem;
        socklen_t addrlenthem;
        int newfd;
        int bytes;
        char buf[200];
        const char * httpRtn = NULL;
        httpInformation info; 

        TEST_PRINT("Attempting accept.");

        if (-1 == (newfd = accept(sockfd, &addrthem, &addrlenthem)))
        {
            TEST_PRINT("accept failed.");
            break;
        }

        TEST_PRINT("accept ok");

        if ((bytes = recv(newfd, buf, sizeof(buf), 0)) <= 0)
        {
            TEST_PRINT("recv failed.");
            break;
        }

        TEST_PRINT_ARG("recv returns with %d",bytes);

        if (NULL == ((httpRtn = clarityProcess(control, &info, buf, bytes, NULL))))
        {
            TEST_PRINT("httpParse returned NULL");
            break;
        }
        sleep(2);
    }

    /* error handling */
    close(sockfd);
}


int main(void) 
{
    controlInformation control;
    setupControl(&control);

    runServer(&control);
    TEST_PRINT("Exiting");
    
    
    return 0;
}

