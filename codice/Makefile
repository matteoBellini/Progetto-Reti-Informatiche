# Compilatore
CC = gcc

# Opzioni
OPT = -Wall

# File sorgenti
SERVER_SRC = server.c modules/quiz.c modules/db.c modules/utils.c modules/gioco.c
CLIENT_SRC = client.c modules/utils.c

# File oggetto
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# File eseguibili
SERVER_BIN = server
CLIENT_BIN = client

# Regola di default
all: $(SERVER_BIN) $(CLIENT_BIN)

# Compilazione server
$(SERVER_BIN) : $(SERVER_OBJ)
	$(CC) -o $@ $^

# Compilazione client
$(CLIENT_BIN) : $(CLIENT_OBJ)
	$(CC) -o $@ $^

# Regola per costruire i file oggetto
%.o : %.c
	$(CC) $(OPT) -c -o $@ $<

# Pulizia dei file eseguibili
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER_BIN) $(CLIENT_BIN)