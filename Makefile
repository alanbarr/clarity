all:

	gcc test.c src/http_handler.c lib/http-parser/http_parser.c -Ilib/http-parser -Isrc -o test -g -Wall -std=c99

