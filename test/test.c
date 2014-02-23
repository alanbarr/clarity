#include <string.h>
#include <stdio.h>
#if 0
#include "http_parser.h"
#endif
#include "http_handler.h"

#define TEST_PRINT(S) \
    printf("(%s:%d) " S "\n", __FILE__, __LINE__)

#define TEST_PRINT_ARG(S, ...) \
    printf("(%s:%d) " S "\n", __FILE__, __LINE__, __VA_ARGS__)

#define ERROR_CHECK(BOOL, STR) \
    test_check(&testData, BOOL, STR, __FILE__, __LINE__)

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
 
static const char * startLinesValid[] = {
    "GET / HTTP/1.1 \r\n",
    "HEAD /robots.txt HTTP/0.9 \r\n",
    "PUT /one/two/three/four HTTP/2.0\r\n",
    "POST / HTTP/0.9 \r\n",
    "TRACE / HTTP/0.9 \r\n",
    "OPTIONS /onetwothreefourfivesix HTTP/0.9 \r\n",
    "DELETE /robots.txt HTTP/0.9 \r\n"
};
    
static const char * startLinesInvalid[] = {
    /* No space between method and resource */
    "GET/ HTTP/1.1 \r\n",    
    /* No CRLF */
    "HEAD /robots.txt HTTP/0.9",
    /* No resource */
    "PUT HTTP/2.0\r\n",
    /* No sapce between resource and version */
    "POST /HTTP/0.9\r\n",
    /* Unknown method */
    "TR / HTTP/0.9 \r\n",
    /* Unknown method */
    "OPTION /onetwothreefourfivesix HTTP/0.9 \r\n",
    /* Newline missing */
    "DELETE /robots.txt HTTP/0.9 \r"
};

static const char * headersValid[] = {
    "DELETE /robots.txt HTTP/0.9 \r\n"
    "HEADER VALUE: HEADER FIELD\r\n"
    "\r\n",
    "DELETE /robots.txt HTTP/0.9 \r\n"
    "HEADER VALUE: HEADER FIELD\r\n"
    "HEADER VALUE: HEADER FIELD\r\n"
    "\r\n"
};


static httpParser startLineDataValid[] = {
    {   GET, 
        { NULL, 1 },
        { 1, 1 }
    },
    {   HEAD, 
        { NULL, 11 },
        { 0, 9 }
    },
    {   PUT,
        { NULL, 19 }, 
        { 2, 0 }
    },
    {   POST,
        { NULL, 1 }, 
        { 0, 9 } 
    },
    {   TRACE,
        { NULL, 1 },
        { 0, 9  }
    },
    {   OPTIONS, 
        { NULL,  23 },
        { 0,  9 }
    },
    {   DELETE,
        { NULL, 11 },
        { 0, 9 }
    }
};


typedef struct {
    uint32_t checks;
    uint32_t passes;
    uint32_t fails;
} testInformation;

static testInformation testData;

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
    PRINT_LINE("in MCB CB!!");
    return 0;
}   

uint32_t getRoot(const char * body, const char * data, const uint16_t size, void * user)
{

#if 0
    static char * html = 
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
        "<html><head>"
        "<title>403 Forbidden</title>"
        "</head><body>"
        "<h1>Forbidden</h1>"
        "<p>You don't have permission to access /"
        "on this server.</p>"
        "</body></html>";
#endif


    PRINT_LINE("IN GET ROOT");

    return 0;
}   


static void test_check(testInformation * test, 
                       bool error, char * string,
                       char * file, int line)
{
    test->checks++;

    if (error)
    {
        printf("CHECK FAILED (%s:%d): %s\n", file,line,string);
        test->fails++;
    }
    else
    {
        test->passes++;
    }
}

static void test_print(testInformation * test)
{
    printf("--Test Information--\n");
    printf("Checks: %d\n", test->checks);
    printf("Passes: %d\n", test->passes);
    printf("Fails: %d\n", test->fails);
}

static int test_startLinesValid(void)
{
    int i;
    httpParser par;
    const char * rtn = NULL;
 
    startLineDataValid[0].resource.data = startLinesValid[0] + 4;  
    startLineDataValid[1].resource.data = startLinesValid[1] + 5;  
    startLineDataValid[2].resource.data = startLinesValid[2] + 4;  
    startLineDataValid[3].resource.data = startLinesValid[3] + 5;  
    startLineDataValid[4].resource.data = startLinesValid[4] + 6;  
    startLineDataValid[5].resource.data = startLinesValid[5] + 8;  
    startLineDataValid[6].resource.data = startLinesValid[6] + 7;  

    for(i=0;i<7;i++)
    {
        memset(&par, 0, sizeof(par));

        rtn = parseHttp(&par, startLinesValid[i], strlen(startLinesValid[i]), NULL);

        ERROR_CHECK(rtn != NULL,"RTN was not NULL");
        ERROR_CHECK(memcmp(&par, &startLineDataValid[i], sizeof(par)) != 0, "memcmp failed");
    }
    return 1;
}

static int test_startLinesInvalid(void)
{
    int i;
    httpParser par;
    const char * rtn = NULL;
 
    for (i=0;i<7;i++)
    {
        memset(&par, 0, sizeof(par));
        rtn = parseHttp(&par, startLinesInvalid[i], strlen(startLinesInvalid[i]), NULL);
        ERROR_CHECK(rtn != NULL,"RTN was not NULL");
    }
    return 1;
}

static int test_headersValid(void)
{
    int index;
    httpParser par;
    const char * rtn = NULL;
    for (index = 0; index<2; index++)
    {
        rtn = parseHttp(&par, headersValid[index], strlen(headersValid[index]), NULL);
        ERROR_CHECK(rtn == NULL,"RTN was NULL");
    }
    return 1;
}


int main(void) 
{
#if 0
    httpParser par;
#endif 
    registerControl(&control);
#if 0
    parseHttp(tst_get, strlen(tst_get), NULL);
#endif
#if 0
    parseHttp(&par, tst_put, strlen(tst_put), NULL);
#endif
#if 0
    parseHttp(&par, tst_root, strlen(tst_root), NULL);
#endif

    test_startLinesValid();
    test_startLinesInvalid();
    test_headersValid();
    test_print(&testData);
}

