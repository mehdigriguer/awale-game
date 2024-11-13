# Compiler
CC = gcc

# Source files
CLIENT_SRC = Client/client2.c
SERVER_SRC = Serveur/server2.c
GAME_SRC = Serveur/game_logic.c

# Executable names
CLIENT_EXEC = client_executable
SERVER_EXEC = server_executable

# Build targets
all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) -o $(CLIENT_EXEC) $(CLIENT_SRC)

$(SERVER_EXEC): $(SERVER_SRC) $(GAME_SRC)
	$(CC) -o $(SERVER_EXEC) $(GAME_SRC) $(SERVER_SRC)

# Clean the build
clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(CLIENT_EXEC) $(SERVER_EXEC)

# Phony targets
.PHONY: all clean