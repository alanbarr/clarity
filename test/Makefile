SRC_DIR=../src
API_DIR=../api
LIB_DIR=../lib

CC=gcc
#CFLAGS=-Wall -Werror -std=c99 -g 
#CFLAGS=-Wall -std=c99 -g -pedantic
CFLAGS=-Wall -g -pedantic

#CLARITY_SRC= $(SRC_DIR)/http_handler.c \
#			 $(LIB_DIR)/http-parser/http_parser.c
#
CLARITY_SRC= $(SRC_DIR)/http_parser.c $(SRC_DIR)/http_handler.c
CLAITY_INC= -I$(SRC_DIR) -I$(API_DIR)

JOY_SRC= $(LIB_DIR)/http-parser/http_parser.c
JOY_INC= -I$(LIB_DIR)/http-parser

all: test_parser server

test_parser: test_parser.c $(CLARITY_SRC)
	$(CC) $(CFLAGS) $< $(CLARITY_SRC) $(CLAITY_INC) -o $@

server: test_server test_server_client

test_server_client: test_server_client.c
	$(CC) $(CFLAGS) $< -o $@

test_server: test_server.c $(CLARITY_SRC)
	$(CC) $(CFLAGS) $< $(CLARITY_SRC) $(CLAITY_INC) -o $@

size_joy: test_size_joy.c
	$(CC) $(CFLAGS) $< $ $(JOY_SRC) $(JOY_INC) -o $@

size_mine: test_size_mine.c
	$(CC) $(CFLAGS) $< $ $(CLARITY_SRC) $(CLAITY_INC) -o $@


clean:
	rm -f test_parser
	rm -f test_server
	rm -f test_server_client

.PHONY: clean
