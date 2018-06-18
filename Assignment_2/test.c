#include <stdio.h>

extern char** environ;
int main(int argc, char* argv[]){
    int index = 0;
    printf("this is test.\n");
    while(argv[index])
	printf("[%s]\n", argv[index++]);
}
