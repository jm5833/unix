#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
int main(int argc, char* argv[]){
	struct stat *buf = malloc(sizeof(stat));
	int fd = open("Makefile", O_RDONLY);
	if(fstat(fd, buf) == -1){
		printf("error with fstat");
		exit(1);
	}
	printf("%d\n", buf->st_size);
	return 1;
}
