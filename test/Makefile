SRC_DIR=../src
LIB_DIR=../lib

CC=gcc
#CFLAGS=-Wall -Werror -std=c99 -g 
#CFLAGS=-Wall -std=c99 -g -pedantic
CFLAGS=-Wall -g -pedantic

#CLARITY_SRC= $(SRC_DIR)/http_handler.c \
#			 $(LIB_DIR)/http-parser/http_parser.c
#
CLARITY_SRC= $(SRC_DIR)/http_handler.c
CLAITY_INC= -I$(SRC_DIR)

JOY_SRC= $(LIB_DIR)/http-parser/http_parser.c
JOY_INC= -I$(LIB_DIR)/http-parser

all: test_unit server

test_unit: test.c $(CLARITY_SRC)
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
	rm test

.PHONY: clean
