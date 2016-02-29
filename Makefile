# Makefile for the Hail protocl and demonstration endpoints
# Stella Chung and Ky-Cuong Huynh
# 8 February 2016

CC = gcc
FLAGS = -std=gnu99 -Wall -Wextra

# Prepend 'server' or 'client' for faster tab auto-complete
SERVER_NAME = server-hail
CLIENT_NAME = client-hail
TEST_NAME = test-hail

SERVER_SRC = server.c hail.c
CLIENT_SRC = client.c hail.c
TEST_SRC = test.c hail.c

all: server client
    
server:
	# Use no optimization by default to avoid introducing regressions
	$(CC) $(FLAGS) -O0 -o $(SERVER_NAME) $(SERVER_SRC)

client:
	$(CC) $(FLAGS) -O0 -o $(CLIENT_NAME) $(CLIENT_SRC)

test:
	$(CC) $(FLAGS) -ggdb -o $(TEST_NAME) $(TEST_SRC)
	./$(TEST_NAME)

clean:
	rm -rf *.o a.out *.dSYM $(SERVER_NAME) $(CLIENT_NAME) $(TEST_NAME)
