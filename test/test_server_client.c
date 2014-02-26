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

#define ADDR_STR "localhost"
#define PORT_STR "56123"

#define TEST_PRINT(S) \
    printf("TEST (%s:%d) " S "\n", __FILE__, __LINE__)

#define TEST_PRINT_ARG(S, ...) \
    printf("TEST (%s:%d) " S "\n", __FILE__, __LINE__, __VA_ARGS__)
 
static const char * test_get =
    "GET / HTTP/1.0\r\n"          
    "Host: localhost\r\n"       
    "User-Agent: C\r\n"             
    "\r\n";


static void runClient(void)
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
        TEST_PRINT("Socket failed.");
    }

    else if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) 
    {
        TEST_PRINT("Connect failed.");
    }
    else if (send(sockfd, test_get, strlen(test_get), 0) == -1)
    {
        TEST_PRINT("Send failed.");
    }
    else
    {
        TEST_PRINT("Done!");
    }

    close(sockfd);
}


int main(void) 
{
    runClient();
    return 0;
}

