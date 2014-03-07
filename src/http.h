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

#ifndef __HTTP_H__
#define __HTTP_H__

#define HTTP_VERSION_STR            "HTTP/1.0"

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



#endif /* __HTTP_H__ */

