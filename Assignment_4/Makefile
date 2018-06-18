FLAGS = -std=c99 -O1 -Wall -Werror -g -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500
all: shell
workingshell: working-shell.c
	gcc working-shell.c -o working-shell ${FLAGS}
shell: shell.c
	gcc shell.c -o shell ${FLAGS}
clean:
	rm -f shell
