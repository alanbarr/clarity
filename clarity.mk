# CLARITY_DIR Externally defined.
CLARITY_SRC = $(CLARITY_DIR)/lib/http-parser/http_parser.c \
			  $(CLARITY_DIR)/src/http_handler.c

CLARITY_INC = $(CLARITY_DIR)/src/ \
			  $(CLARITY_DIR)/api/ \
			  $(CLARITY_DIR)/lib/http-parser

