#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>

#define DEFAULT_TOKENLIST_SIZE 10

char *readline();
void execute(char **tlist);
char **tokenizelist(char *line);
void changedir(char **tlist);
char *resolvepath(char *path);
int search(char **tlist, char *delimiter);
char **createargs(char **tlist, int in, int out, int outa, int err);
void freelist(char **list);
int listsize(char **list);

int main(int argc, char* argv[]){
	char *PS1;
	char *line;
	char **tlist;
	PS1 = getenv("PS1");
	if(!PS1) PS1 = ">>> ";
	while(1){
		printf("%s ", PS1);
		line = readline();
		if(!line) continue;
		tlist = tokenizelist(line);
		free(line);
		execute(tlist);
		//freelist(tlist);
	}
	
}
//gets input from the user
char *readline(){
	char *line;
	size_t size = 0;
	if(getline(&line, &size, stdin) == -1){
		printf("error with getline\n");	
		exit(1);
	}
	if(strlen(line) == 1)
		return NULL;
	return line;
}
//executes the commands listed in tlist
void execute(char **tlist){
	char **args;
	int status, in, out, outa, err;
	if(strcasecmp(tlist[0], "cd") == 0){
		changedir(tlist);
		return;
	}
	else if(strcasecmp(tlist[0], "exit") == 0){
		freelist(tlist);
		exit(0);
	}
	
	pid_t pid = fork();
	if(pid < 0)
		perror("Error forking\n");
	else if(pid == 0){
		in = search(tlist, "<");
		out = search(tlist, ">");
		err = search(tlist, "2>");
		outa = search(tlist, ">>");
		
		if(in >= 0){
			int fd0 = open(tlist[in + 1], O_RDONLY);
			if(fd0 < 0){
				printf("shell: %s\n", strerror(errno));
				exit(1);
			}
			dup2(fd0, 0);
			close(fd0);
		}
		if(out >= 0){
			int fd1 = open(tlist[out + 1], O_CREAT | O_WRONLY, 0644);
			if(fd1 < 0){
				perror("shell");
				exit(1);
			}
			dup2(fd1, 1);
			close(fd1);
		}
		else if(outa >= 0){
			int fd1 = open(tlist[outa + 1], O_RDWR | O_APPEND);
			if(fd1 < 0){
				perror("shell");
				exit(1);
			}
			dup2(fd1, 1);
			close(fd1);
		}
		if(err >= 0){
			int fd2 = open(tlist[err + 1], O_CREAT | O_WRONLY, 0644);
			if(fd2 < 0){
				perror("shell");
				exit(1);
			}
			dup2(fd2, 2);
			close(fd2);
		}
		args = createargs(tlist, in, out, outa, err);
		if(execvp(args[0], args) == -1){
			perror("shell");
			exit(1);
		}
	}
	else
		wait(&status);
}
//tokenizes line by a space delimiter
char **tokenizelist(char *line){
	char **tlist;
	char *token, *delimiter = " \t\n\r";
	int index = 0, size = DEFAULT_TOKENLIST_SIZE;
	tlist = malloc(sizeof(char*) * size);
	if(!tlist){
		printf("Error with malloc\n");
		exit(1);
	}
	token = strtok(line, delimiter);
	if(!token) return tlist;
	while(token != NULL){
		tlist[index] = malloc(strlen(token) + 1);
		if(!tlist[index]){
			printf("Error with malloc\n");
			exit(1);
		}
		strcpy(tlist[index], token);
		index++;
		if(index > size){
			size = size * 2;
			tlist = realloc(tlist, size);
			if(!tlist) printf("error with realloc\n");
		}
		token = strtok(NULL, delimiter);
	}
	return tlist;
}
//cd for my shell. assumes the HOME env variable is set. 
void changedir(char **tlist){
	char *home = getenv("HOME");
	if(tlist[1] == NULL){
		if(chdir(home) == -1)
			printf("shell ");
		return;
	}
	//checking if the path given has the tilde that indicates the home directory
	if(strncmp(tlist[1], "~", 1) == 0){
		if(!home){
			printf("$HOME isn't set.\n");
			exit(1);
		}
		char *abspath = resolvepath(tlist[1]);
		if(!abspath) return;
		if(chdir(abspath) == -1)
			perror("shell ");
		free(abspath);
		return;
	}
	printf("%s\n", tlist[1]);
	if(chdir(tlist[1]) == -1)
		perror("shell");
}
//converts partial paths into absolute paths
//eg. input: ~/Unix/homework 
//output: /home/jack/Unix/homework.
//relies on $HOME being set. also assumes that ~ is always the first char
char *resolvepath(char *path){
	char *abspath, *relpath;
	char *home = getenv("HOME");
	abspath = malloc(strlen(path) + strlen(home) + 1);
	if(!abspath){
		printf("Error with malloc\n");
		return NULL;
	}
	relpath = malloc(strlen(path) + 1);
	if(!relpath){
		printf("Error with malloc\n");
		free(abspath);
		return NULL;
	}
	strcpy(relpath, path);
	strcpy(abspath, home);
	strcat(abspath, relpath + 1);
	free(relpath);
	return abspath;
}
//returns the index of the first instance of delimiter in tlist
//returns -1 if the delimiter isn't found
int search(char **tlist, char *delimiter){
	int i = 0;
	while(tlist[i]){
		if(strcmp(tlist[i], delimiter) == 0)
			return i;
		i++;
	}
	return -1;
}
//create an array to be used as the argument list for execvp
//omits io redirection characters and the file names that follow it
char **createargs(char **tlist, int in, int out, int outa, int err){
	if(in == -1 && out == -1 && outa == -1 && err == -1)
		return tlist;
	int size = listsize(tlist);
	char **list = malloc(sizeof(char*) * size);
	if(!list){
		printf("Error with malloc.\n");
		return NULL;
	}
	int x = 0, y = 0;
	while(tlist[x] && y < size){
		if(x == in || x == out || x == outa || x == err){
			x+=2;
			continue;
		}
		list[y] = malloc(sizeof(tlist[x]));
		strcpy(list[y], tlist[x]);
		x++;
		y++;
	}
	freelist(tlist);
	return list;
}
//frees up the list thats been passed in
void freelist(char **list){
	int i = 0;
	while(list[i]){
		free(list[i]);
		i++;
	}
	free(list);
}
//returns the size of the passed in list
int listsize(char **list){
	int size = 0;
	while(list[size])
		size++;
	return size;
}

