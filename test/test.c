#include <string.h>
#include <stdio.h>

#if 0
#include "http_parser.h"
#endif
#include "http_handler.h"

#define TEST_PRINT(S) \
    printf("TEST (%s:%d) " S "\n", __FILE__, __LINE__)

#define TEST_PRINT_ARG(S, ...) \
    printf("TEST (%s:%d) " S "\n", __FILE__, __LINE__, __VA_ARGS__)

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
    TEST_PRINT("in MCB CB!!");
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


    TEST_PRINT("IN GET ROOT");

    return 0;
}   


static void test_check(testInformation * test, 
                       bool error, char * string,
                       char * file, int line)
{
    test->checks++;

    if (error)
    {
        printf("FAILED (%s:%d): %s\n", file,line,string);
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

    static const char * startLinesValid[] = {
        "GET / HTTP/1.1 \r\n"
         "\r\n",
        "HEAD /robots.txt HTTP/0.9 \r\n"
         "\r\n",
        "PUT /one/two/three/four HTTP/2.0\r\n"
         "\r\n",
        "POST / HTTP/0.9 \r\n"
         "\r\n",
        "TRACE / HTTP/0.9 \r\n"
         "\r\n",
        "OPTIONS /onetwothreefourfivesix HTTP/0.9 \r\n"
         "\r\n",
        "DELETE /robots.txt HTTP/0.9 \r\n"
         "\r\n"
    };

    static httpParser startLineDataValid[7];

    memset(&startLineDataValid, 0, sizeof(startLineDataValid));

    startLineDataValid[0].type = GET; 
    startLineDataValid[0].resource.size = 1; 
    startLineDataValid[0].resource.data = startLinesValid[0] + 4;  
    startLineDataValid[0].version.major = 1; 
    startLineDataValid[0].version.minor = 1; 

    startLineDataValid[1].type = HEAD; 
    startLineDataValid[1].resource.size = 11; 
    startLineDataValid[1].resource.data = startLinesValid[1] + 5;  
    startLineDataValid[1].version.major = 0; 
    startLineDataValid[1].version.minor = 9; 

    startLineDataValid[2].type = PUT; 
    startLineDataValid[2].resource.size = 19; 
    startLineDataValid[2].resource.data = startLinesValid[2] + 4;  
    startLineDataValid[2].version.major = 2; 
    startLineDataValid[2].version.minor = 0; 

    startLineDataValid[3].type = POST; 
    startLineDataValid[3].resource.size = 1; 
    startLineDataValid[3].resource.data = startLinesValid[3] + 5;  
    startLineDataValid[3].version.major = 0; 
    startLineDataValid[3].version.minor = 9; 

    startLineDataValid[4].type = TRACE; 
    startLineDataValid[4].resource.size = 1; 
    startLineDataValid[4].resource.data = startLinesValid[4] + 6;  
    startLineDataValid[4].version.major = 0; 
    startLineDataValid[4].version.minor = 9; 
 
    startLineDataValid[5].type = OPTIONS; 
    startLineDataValid[5].resource.size = 23; 
    startLineDataValid[5].resource.data = startLinesValid[5] + 8;  
    startLineDataValid[5].version.major = 0; 
    startLineDataValid[5].version.minor = 9; 

    startLineDataValid[6].type = DELETE; 
    startLineDataValid[6].resource.size = 11; 
    startLineDataValid[6].resource.data = startLinesValid[6] + 7;  
    startLineDataValid[6].version.major = 0; 
    startLineDataValid[6].version.minor = 9; 

    for(i=0;i<7;i++)
    {
        TEST_PRINT_ARG("loop iternation %d.", i);
        memset(&par, 0, sizeof(par));

        rtn = parseHttp(&par, startLinesValid[i], strlen(startLinesValid[i]),
                        NULL);

        ERROR_CHECK(rtn == NULL,"RTN was NULL");
     
        ERROR_CHECK(startLineDataValid[i].type != par.type,
                    "Type not correct.");
        ERROR_CHECK(startLineDataValid[i].resource.size != par.resource.size,
                    "Resource size not correct.");
        ERROR_CHECK(startLineDataValid[i].resource.data != par.resource.data,
                    "Resource data not correct.");  
        ERROR_CHECK(startLineDataValid[i].version.major != par.version.major,
                    "Version major not correct."); 
        ERROR_CHECK(startLineDataValid[i].version.minor != par.version.minor,
                    "Version minor not correct."); 
    }
    return 1;
}

static int test_startLinesInvalid(void)
{
    int i;
    httpParser par;
    const char * rtn = NULL;
    
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



    
    for (i=0;i<7;i++)
    {
        memset(&par, 0, sizeof(par));
        rtn = parseHttp(&par, startLinesInvalid[i], strlen(startLinesInvalid[i]),
                        NULL);
        ERROR_CHECK(rtn != NULL,"RTN was not NULL");
    }
    return 1;
}

static int test_headersValid(void)
{
    int index;
    int hindex;
    httpParser par;
    const char * rtn = NULL;
    static httpParser headerData[3];

    static const char * headersValid[] = {
    "DELETE /robots.txt HTTP/0.9 \r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "\r\n",
    "DELETE /robots.txt HTTP/0.9 \r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "\r\n",

    "GET / HTTP/2.0 \r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "\r\n",

    "PUT /robots.txt HTTP/0.9 \r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 1\r\n"
    "\r\n"
    "1",
    };

    memset(&headerData, 0, sizeof(headerData));

    headerData[0].type = DELETE; 
    headerData[0].resource.size = 11; 
    headerData[0].resource.data = headersValid[0] + 7;  
    headerData[0].version.major = 0; 
    headerData[0].version.minor = 9; 
    headerData[0].headers[0].field.data = headersValid[0] + 30; 
    headerData[0].headers[0].field.size = 12; 
    headerData[0].headers[0].value.data = headersValid[0] + 44; 
    headerData[0].headers[0].value.size = 12; 

    headerData[1].type = DELETE; 
    headerData[1].resource.size = 11; 
    headerData[1].resource.data = headersValid[1] + 7;  
    headerData[1].version.major = 0; 
    headerData[1].version.minor = 9; 
    headerData[1].headers[0].field.data = headersValid[1] + 30; 
    headerData[1].headers[0].field.size = 12; 
    headerData[1].headers[0].value.data = headersValid[1] + 30 + (14); 
    headerData[1].headers[0].value.size = 12; 
    headerData[1].headers[1].field.data = headersValid[1] + 30 + (14*2); 
    headerData[1].headers[1].field.size = 12; 
    headerData[1].headers[1].value.data = headersValid[1] + 30 + (14*3); 
    headerData[1].headers[1].value.size = 12; 
 
    headerData[2].type = GET; 
    headerData[2].resource.size = 1; 
    headerData[2].resource.data = headersValid[2] + 4;  
    headerData[2].version.major = 2; 
    headerData[2].version.minor = 0; 
    headerData[2].headers[0].field.data = headersValid[2] + 17; 
    headerData[2].headers[0].field.size = 12; 
    headerData[2].headers[0].value.data = headersValid[2] + 17 + (14); 
    headerData[2].headers[0].value.size = 12; 
    headerData[2].headers[1].field.data = headersValid[2] + 17 + (14*2); 
    headerData[2].headers[1].field.size = 12; 
    headerData[2].headers[1].value.data = headersValid[2] + 17 + (14*3); 
    headerData[2].headers[1].value.size = 12; 
    headerData[2].headers[2].field.data = headersValid[2] + 17 + (14*4); 
    headerData[2].headers[2].field.size = 12; 
    headerData[2].headers[2].value.data = headersValid[2] + 17 + (14*5);
    headerData[2].headers[2].value.size = 12; 
    headerData[2].headers[3].field.data = headersValid[2] + 17 + (14*6);
    headerData[2].headers[3].field.size = 12; 
    headerData[2].headers[3].value.data = headersValid[2] + 17 + (14*7);
    headerData[2].headers[3].value.size = 12; 
    headerData[2].headers[4].field.data = headersValid[2] + 17 + (14*8);
    headerData[2].headers[4].field.size = 12; 
    headerData[2].headers[4].value.data = headersValid[2] + 17 + (14*9);
    headerData[2].headers[4].value.size = 12; 
    headerData[2].headers[5].field.data = headersValid[2] + 17 + (14*10);
    headerData[2].headers[5].field.size = 12; 
    headerData[2].headers[5].value.data = headersValid[2] + 17 + (14*11);
    headerData[2].headers[5].value.size = 12; 

    headerData[3].type = PUT; 
    headerData[3].resource.size = 11; 
    headerData[3].resource.data = headersValid[3] + 7;  
    headerData[3].version.major = 0; 
    headerData[3].version.minor = 9; 
    headerData[3].content.data = headersValid[3] + 41;
    headerData[3].content.size = 1;

    for (index = 0; index<3; index++)
    {
        TEST_PRINT_ARG("Checking index: %d.", index);

        memset(&par,0,sizeof(par));
        rtn = parseHttp(&par, headersValid[index], strlen(headersValid[index]),
                        NULL);
        ERROR_CHECK(rtn == NULL,"RTN was NULL");  
        ERROR_CHECK(headerData[index].type != par.type,
                    "Type not correct.");
        ERROR_CHECK(headerData[index].resource.size != par.resource.size,
                    "Resource size not correct.");
        ERROR_CHECK(headerData[index].resource.data != par.resource.data,
                    "Resource data not correct.");  
        ERROR_CHECK(headerData[index].version.major != par.version.major,
                    "Version major not correct."); 
        ERROR_CHECK(headerData[index].version.minor != par.version.minor,
                    "Version minor not correct."); 
        ERROR_CHECK(headerData[index].content.size != par.content.size,
                    "content size not correct.");
        ERROR_CHECK(headerData[index].content.data != par.content.data,
                    "content data not correct.");  

        for (hindex = 0; hindex< MAX_HEADERS; hindex++)
        {
            TEST_PRINT_ARG("Checking header index: %d.", hindex);

            ERROR_CHECK(par.headers[hindex].field.data   != 
                        headerData[index].headers[hindex].field.data,
                        "Header field data not as expected");
            ERROR_CHECK(par.headers[hindex].field.size !=
                        headerData[index].headers[hindex].field.size,
                        "Header field size not as expected");
            ERROR_CHECK(par.headers[hindex].value.data   != 
                        headerData[index].headers[hindex].value.data,
                        "Header value data not as expected");
            ERROR_CHECK(par.headers[hindex].value.size !=
                        headerData[index].headers[hindex].value.size,
                        "Header value size not as expected");
        }
    }
    return 1;
}


static int test_headersInvalid(void)
{
    int index;
    int hindex;
    httpParser par;
    const char * rtn = NULL;
    static httpParser headerData[2];

    static const char * headersInvalid[] = {
    "GET / HTTP/1.0 \r\n"
    "HEADER FIELD:\r\n"
    "\r\n",
    "GET / HTTP/1.0 \r\n"
    "HEADER FIELD\r\n"
    "\r\n",
    "GET / HTTP/1.0 \r\n"
    "HEADER FIELD: HEADER VALUE\r\n"
    "HEADER FIELD\r\n"
    "\r\n",
    };

    memset(&headerData, 0, sizeof(headerData));

    headerData[0].type = GET; 
    headerData[0].resource.size = 1; 
    headerData[0].resource.data = headersInvalid[0] + 4;  
    headerData[0].version.major = 1; 
    headerData[0].version.minor = 0; 

    headerData[1].type = GET; 
    headerData[1].resource.size = 1; 
    headerData[1].resource.data = headersInvalid[1] + 4;  
    headerData[1].version.major = 1; 
    headerData[1].version.minor = 0; 

    headerData[2].type = GET; 
    headerData[2].resource.size = 1; 
    headerData[2].resource.data = headersInvalid[2] + 4;  
    headerData[2].version.major = 1; 
    headerData[2].version.minor = 0; 
    headerData[2].headers[0].field.data = headersInvalid[2] + 17; 
    headerData[2].headers[0].field.size = 12; 
    headerData[2].headers[0].value.data = headersInvalid[2] + 17+14; 
    headerData[2].headers[0].value.size = 12; 


    for (index = 0; index<3; index++)
    {
        TEST_PRINT_ARG("Checking index: %d.", index);

        memset(&par,0,sizeof(par));
        rtn = parseHttp(&par, headersInvalid[index], strlen(headersInvalid[index]),
                        NULL);

        ERROR_CHECK(rtn != NULL,"RTN was not NULL");

        ERROR_CHECK(headerData[index].type != par.type,
                    "Type not correct.");
        ERROR_CHECK(headerData[index].resource.size != par.resource.size,
                    "Resource size not correct.");
        ERROR_CHECK(headerData[index].resource.data != par.resource.data,
                    "Resource data not correct.");  
        ERROR_CHECK(headerData[index].version.major != par.version.major,
                    "Version major not correct."); 
        ERROR_CHECK(headerData[index].version.minor != par.version.minor,
                    "Version minor not correct."); 

        for (hindex = 0; hindex< MAX_HEADERS; hindex++)
        {
            TEST_PRINT_ARG("Checking header index: %d.", hindex);

            ERROR_CHECK(par.headers[hindex].field.data   != 
                        headerData[index].headers[hindex].field.data,
                        "Header field data not as expected");
            ERROR_CHECK(par.headers[hindex].field.size !=
                        headerData[index].headers[hindex].field.size,
                        "Header field size not as expected");
            ERROR_CHECK(par.headers[hindex].value.data   != 
                        headerData[index].headers[hindex].value.data,
                        "Header value data not as expected");
            ERROR_CHECK(par.headers[hindex].value.size !=
                        headerData[index].headers[hindex].value.size,
                        "Header value size not as expected");
        }
    }
    return 1;
}


static int test_bodyValid(void)
{
    httpParser par;
    int index;
    const char * rtn;

    static const char * bodiesValid[] = {   
        "PUT / HTTP/1.0\r\n"                
        "Content-Type: text/plain\r\n"      
        "Content-Length: 4\r\n"             
        "\r\n"                              
        "BODY",                             
 
        "POST / HTTP/1.0 \r\n"              
        "HEADER FIELD: HEADER VALUE\r\n"    
        "Content-Type: text/plain\r\n"      
        "Content-Length: 15\r\n"            
        "HEADER FIELD: HEADER VALUE\r\n"    
        "\r\n"                              
        "BODY12345678910"                   
    };

    static httpParser bodyData[2];

    bodyData[0].type = GET; 
    bodyData[0].resource.size = 1; 
    bodyData[0].resource.data = bodiesValid[0] + 4;  
    bodyData[0].version.major = 1; 
    bodyData[0].version.minor = 0; 
    bodyData[0].content.data = bodiesValid[0] + 30;
    bodyData[0].content.size = 10;
    bodyData[0].body.data = bodiesValid[0] + 63; 
    bodyData[0].body.size = 4; 

    bodyData[1].type = POST; 
    bodyData[1].resource.size = 1; 
    bodyData[1].resource.data = bodiesValid[0] + 5;  
    bodyData[1].version.major = 1; 
    bodyData[1].version.minor = 0; 
    bodyData[1].content.data = bodiesValid[1] + 60;
    bodyData[1].content.size = 10;
    bodyData[1].body.data = bodiesValid[1] + 122; 
    bodyData[1].body.size = 15; 

    for (index = 0; index < 2; index++)
    {
        TEST_PRINT_ARG("Checking index: %d.", index);

        memset(&par,0,sizeof(par));
        rtn = parseHttp(&par, bodiesValid[index], strlen(bodiesValid[index]),
                        NULL);
        ERROR_CHECK(rtn == NULL,"RTN was NULL");
        ERROR_CHECK(par.body.data != bodyData[index].body.data,
                    "Body Data not as expected.");
        ERROR_CHECK(par.body.size != bodyData[index].body.size,
                    "Body Size not as expected.");
        ERROR_CHECK(par.content.size != bodyData[index].content.size,
                    "Content size not as expected.");
        ERROR_CHECK(par.content.data != bodyData[index].content.data,
                    "Content data not as expected.");
    }

    return 0;
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
    test_headersInvalid();
    test_bodyValid();
    test_print(&testData);
    return 0;
}

