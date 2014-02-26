#include <string.h>
#include <stdio.h>
#include "http_handler.h"

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

#define PORT 56123
#define PORT_STR "56123"

#define TEST_PRINT(S) \
    printf("TEST (%s:%d) " S "\n", __FILE__, __LINE__)

#define TEST_PRINT_ARG(S, ...) \
    printf("TEST (%s:%d) " S "\n", __FILE__, __LINE__, __VA_ARGS__)

static uint32_t generalPut(const char * body, const char * data, 
                           const uint16_t size, void * user)
{
    return 0;
}   

uint32_t generalGet(const char * body, const char * data,
                    const uint16_t size, void * user)
{
    return 0;
}   

uint32_t generalPost(const char * body, const char * data,
                     const uint16_t size, void * user)
{
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

static void runServer(void)
{
    int sockfd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
#if 0
    struct sockaddr_in addr;
    socklen_t sin_size;
    int rv;
#endif

#if 0
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr
#endif

    memset(&hints, 0 , sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;     

    if (0 != getaddrinfo("localhost", PORT_STR, &hints, &servinfo))
    {
        TEST_PRINT("getAddrInfo failed");
        TEST_PRINT_ARG("errno: %d", errno);
        TEST_PRINT_ARG("errno: %s", strerror(errno));
        return;
    }

    else if (-1 == (sockfd = socket(servinfo->ai_family,
                                    servinfo->ai_socktype,
                                    servinfo->ai_protocol)))
    {
        TEST_PRINT("Socket failed");
        return;
    }

    else if (-1 == (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)))
    { 
        TEST_PRINT("bind failed");
        return;
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
        int i;
        char buf[20];

        if (-1 == (newfd = accept(sockfd, &addrthem, &addrlenthem)))
        {
            TEST_PRINT("accept failed");
            continue;
        }

        TEST_PRINT("accept ok");

        if ((bytes = recv(newfd, buf, sizeof(buf), 0)) <= 0)
        {
            TEST_PRINT("recv failed");
            return;
        }
        TEST_PRINT_ARG("recv returns with %d",bytes);
        for (i=0; i<bytes; i++)
        {
            putchar((int)buf[i]);
        }
        putchar('\n');
        sleep(2);
    }

}


int main(void) 
{
    controlInformation control;
    setupControl(&control);
    httpRegisterControl(&control);

    runServer();
    TEST_PRINT("Exiting");
    
    
    return 0;
}

