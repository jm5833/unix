#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
extern char** environ;

void printenv(char** env);
int  envlen(char** env);
void clean();
void checkmalloc(char* mem);
void parseargs(int argc, char* argv[]);
void allocatepar(int numofvar, char* argv[]);
void allocateful(int numofvar, char* argv[]);
void createproc(int start, char* argv[]);
 
int main(int argc, char* argv[]){
    if(argc == 1)
	printenv(environ);
    else if(argc > 1){
	parseargs(argc, argv);
    }
}

//Prints the environment
void printenv(char** env){
    int i = 0;
    while(env[i])
	printf("%s\n", env[i++]);
}

//returns the length of the environment variable given to it
int envlen(char** env){
    int length = 0;
    while(env[length]) length++;
    return length;
}

//frees up any malloc-ed memory
void clean(){
    int size = envlen(environ);
    for(int i = 0; i < size; i++){
	free(environ[i]);
    }
    free(environ);
}

//checks malloced memory
void checkmalloc(char* mem){
    if(!mem){
	printf("error mallocing\n");
	exit(-1);
    }
}
//Assumes that the -i will always be the second argument
void parseargs(int argc, char* argv[]){
    int i = 1;
    int numofvar = 0, ignore = 0;
    if((strcmp(argv[1], "-i") == 0)){
	i = 2;
	ignore = 1;
    }
    if(ignore && argc == 2)
	exit(1);
    while(argv[i] && strchr(argv[i], '=')){
	numofvar++;
	i++;
    }
    if(ignore)
    	allocatepar(numofvar, argv);
    else
    	allocateful(numofvar, argv);
    if(1 + ignore + numofvar < argc)
	createproc(1 + ignore + numofvar, argv);
    printenv(environ);
    clean();
}

//Create a new environment variable that doesn't use the
//already existing environment variable
void allocatepar(int numofvar, char* argv[]){
    if(numofvar == 0) return;
    int start = 2, i = 0;
    char** newenv = (char**)malloc((numofvar + (numofvar%2)) * sizeof(char*));
    while((i < numofvar) && argv[start] && strchr(argv[start],'=')){
	newenv[i] = (char*)malloc(sizeof(strlen(argv[start])));
	strcpy(newenv[i], argv[start]);
	i++;
	start++;
    }
    environ = newenv;
}

//Create a new enviornment variable that incorporates
//the existing environment variable
void allocateful(int numofvar, char* argv[]){
    if(numofvar == 0) return;
    char** newenv;
    int len = envlen(environ);
    int offset = 0, index = 0, x = 0, vardup = 0;
    if(!(newenv = (char**)malloc((numofvar + len) * sizeof(char*)))){
	printf("error allocating memory\n");
	exit(-1);
    }
    for(int i = 0; i < numofvar; i++)
    {
	newenv[i] = (char*)malloc(strlen(argv[i+1]));
	checkmalloc(newenv[i]);
	strcpy(newenv[i], argv[i+1]);
    }
    while(environ[x]){
	for(int y = 0; y < numofvar; y++){
	    index = (int)(strchr(newenv[y], '=') - newenv[y]);
	    if(strncmp(environ[x], newenv[y], index) == 0)
		vardup = 1;
	}
	if(vardup == 0){
	    newenv[numofvar + offset] = (char*)(malloc(strlen(environ[x])));
	    checkmalloc(newenv[numofvar + offset]);
	    strcpy(newenv[numofvar + offset], environ[x]);
	    offset++;
	}
	vardup = 0;
	x++;
    }
  
    if(!(realloc(newenv, (offset + numofvar) * sizeof(char*)))){
	printf("error reallocating memory.\n");
	exit(-1);
    }
    environ = newenv;
}

//create a new process
void createproc(int start, char* argv[]){
    if(execvp(argv[start], argv+start) == -1){
	printf("error execing\n");
	clean();
	exit(-1);
    }
}
