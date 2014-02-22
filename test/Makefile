SRC_DIR=../src
#LIB_DIR=../lib

CC=gcc
CFLAGS=-Wall -Werror -std=c99 -g 

#CLARITY_SRC= $(SRC_DIR)/http_handler.c \
#			 $(LIB_DIR)/http-parser/http_parser.c
#
CLARITY_SRC= $(SRC_DIR)/http_handler.c

CLAITY_INC= -I$(LIB_DIR)/http-parser	\
			-I$(SRC_DIR)

test: test.c $(CLARITY_SRC)
	$(CC) $(CFLAGS) $< $(CLARITY_SRC) $(CLAITY_INC) -o $@

clean:
	rm test

.PHONY: clean
