# Makefile for the Hail protocl and demonstration endpoints
# Stella Chung and Ky-Cuong Huynh
# 8 February 2016

CC = gcc
FLAGS = -std=gnu99 -Wall -Wextra -ggdb
# -lm is the math library
# See: https://stackoverflow.com/questions/11336477/gcc-will-not-properly-include-math-h
LIBRARIES = -lm

# Prepend 'server' or 'client' for faster tab auto-complete
SERVER_NAME = server-hail
CLIENT_NAME = client-hail
TEST_NAME = test-hail

SERVER_SRC = server.c hail.c
CLIENT_SRC = client.c hail.c
TEST_SRC = test.c hail.c

all: server client
    
# Use enough optimization by default to be warned about uninitialized variables
server:
	$(CC) $(FLAGS) -O2 -o $(SERVER_NAME) $(SERVER_SRC) $(LIBRARIES)

client:
	$(CC) $(FLAGS) -O2 -o $(CLIENT_NAME) $(CLIENT_SRC) $(LIBRARIES)

test: clean all
	mkdir test/
	mv $(CLIENT_NAME) test/
	./$(SERVER_NAME) 4242 4096 &
	cd test/ && ./$(CLIENT_NAME) localhost 4242 LICENSE

clean:
	rm -rf test/
	rm -rf *.o a.out *.dSYM $(SERVER_NAME) $(CLIENT_NAME) $(TEST_NAME)
