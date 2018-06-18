FLAGS = -std=c99 -O1 -Wall -Werror -g -D_POSIX_C_SOURCE=200809L -Wextra -pedantic
all: server client
server: server.c
	gcc server.c -o server ${FLAGS}
client:
	gcc client.c -o client ${FLAGS}
clean:
	rm -f server client
