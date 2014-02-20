#include <string.h>
#include <stdio.h>
#include "http_parser.h"
#include "http_handler.h"

char * tst_get =
"GET /robots.txt HTTP/1.0\r\n"
"Host: www.joes-hardware.com\r\n"
"User-Agent: Slurp/2.0\r\n"
"Date: Wed Oct 3 20:22:48 EST 2001\r\n"
"\r\n";

char * tst_put =
"POST /path/script.cgi HTTP/1.0\r\n"
"From: frog@jmarshall.com\r\n"
"User-Agent: HTTPTool/1.0\r\n"
"Content-Type: application/x-www-form-urlencoded\r\n"
"Content-Length: 32\r\n"
"\r\n"
"home=Cosby&favorite+flavor=flies\r\n";

char * tst_root =
"GET / HTTP/1.0\r\n"
"\r\n";



uint32_t mcb(const char * body, const char * data, const uint16_t size, void * user);
uint32_t getRoot(const char * body, const char * data, const uint16_t size, void * user);

controlInformation control =  /* TODO only one? */
{
    "Test_Control",     /* Device Name */
    /* Resources */
    {

        /* Resource 1 */
        {
            "/",
            GET,
            /* Methods */
            {
                {
                    GET,
                    getRoot
                }
            }
        },
        /* Resource 2 */
        {
            "/path/script.cgi", /* Resource Name */
            POST,               /* Methods Supported Mask */
            /* Methods */
            {
                /* Method 1 */
                {
                    POST,       /* Method Type */
                    mcb         /* Callback */
                }
            }
        },
        /* Resource 3 */
        {
            "/robots.txt",   /* Resource Name */
            GET | PUT,       /* Methods Supported Mask */
            /* Methods */
            {
                /* Method 1 */
                {
                    GET,            /* Method Type */
                    mcb             /* Callback */
                },
                /* Method 2 */
                {
                    PUT,            /* Method Type */
                    mcb             /* Callback */
                }
            },
        }

    },
    {
    }
};



uint32_t mcb(const char * body, const char * data, const uint16_t size, void * user)
{
    PRINT_LINE("in MCB CB!!", NULL);
    return 0;
}   

uint32_t getRoot(const char * body, const char * data, const uint16_t size, void * user)
{

static char * html = 
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
        "<html><head>"
        "<title>403 Forbidden</title>"
        "</head><body>"
        "<h1>Forbidden</h1>"
        "<p>You don't have permission to access /"
        "on this server.</p>"
        "</body></html>";

    PRINT_LINE("IN GET ROOT", NULL);

    return 0;
}   



int main(void) 
{
    registerControl(&control);
#if 0
    parseHttp(tst_get, strlen(tst_get), NULL);
#endif
#if 0
    parseHttp(tst_put, strlen(tst_put), NULL);
#endif
#if 1
    parseHttp(tst_root, strlen(tst_root), NULL);
#endif
}

