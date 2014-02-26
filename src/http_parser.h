#ifndef __HTTP_PARSER__
#define __HTTP_PARSER__

#include "clarity_api.h"


#if 0
#define RECEIVE_BUFFER_SIZE         1024
#define RECEIVE_BUFFER_LAST_INDEX   (RECEIVE_BUFFER_SIZE - 1)

#define TRANSMIT_BUFFER_SIZE        1024
#endif

#define HTTP_EOL_STR                "\r\n"
#define HTTP_EOL_LEN                2

#define HTTP_CONTENT_TYPE_STR       "Content-Type: "
#define HTTP_CONTENT_TYPE_LEN       14

#define HTTP_CONTENT_LENGTH_STR     "Content-Length: " 
#define HTTP_CONTENT_LENGTH_LEN     16


#define HTTP_VERSION_LEN            8   /* "HTTP/X.X" */
                                    /* "GET / " VERSION \r\n */
#define HTTP_START_LINE_MIN_LEN     (6 + HTTP_VERSION_LEN) 
#define HTTP_SPACE_LEN              1   /* ' ' */

#define ASCII_DIGIT_OFFSET          48

#define HTTP_RESOURCE_START_CHAR    '/'

/* unsigned only */
#define SMALLER(X,Y) ((uint32_t)(X)<(uint32_t)(Y) ? (uint32_t)X : (uint32_t)Y)

/* Length of string offset from an offset when  string start and length
 * are known*/
#define P_OFFSET_LEN(START, LENGTH, OFFSET)   ((START+LENGTH) - (OFFSET))

/* TODO macro for pointer length via subtraction. */
#define P_DIFF_LEN(BIGGER, SMALLER) (BIGGER - SMALLER + 1)



#endif /* __HTTP_PARSER__ */

