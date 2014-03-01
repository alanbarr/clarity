SRC_DIR=../src
API_DIR=../api
OUT_DIR=output

VPATH=$(OUT_DIR)

CC=gcc
#CFLAGS=-Wall -Werror -std=c99 -g 
#CFLAGS=-Wall -std=c99 -g -pedantic
CFLAGS=-Wall -g -pedantic

CLARITY_SRC= $(SRC_DIR)/http_parser.c $(SRC_DIR)/http_handler.c
CLAITY_INC= -I$(SRC_DIR) -I$(API_DIR)

all: $(OUT_DIR) test_parser test_server

test_parser: test_parser.c $(CLARITY_SRC)
	$(CC) $(CFLAGS) $< $(CLARITY_SRC) $(CLAITY_INC) -o $(OUT_DIR)/$@

test_server: test_server_server test_server_client

test_server_client: test_server_client.c
	$(CC) $(CFLAGS) $< -o $(OUT_DIR)/$@

test_server_server: test_server_server.c $(CLARITY_SRC)
	$(CC) $(CFLAGS) $< $(CLARITY_SRC) $(CLAITY_INC) -o $(OUT_DIR)/$@

$(OUT_DIR):
	mkdir -p $(OUT_DIR)


clean:
	#rm -r ${OUT_DIR)

.PHONY: clean
