# Makefile for Socket Programming Assignment 0

CC = gcc
CFLAGS = -Wall -Wextra -O2

# Source files
CLIENT_SRC = client.c
SERVER_SRC = server.c

# Output binaries
CLIENT_BIN = client
SERVER_BIN = server

# Default target
all: $(CLIENT_BIN) $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_SRC)

# Remove compiled files
clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)

# Rebuild everything
rebuild: clean all

.PHONY: all clean rebuild
