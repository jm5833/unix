#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

//struct to hold the list of inodes
//duplist is a pointer to the actual list of inodes
//maxSize is the size of which duplist was malloc-ed with
//currentSize is the amount of inodes currently in duplist
struct duplicate{
	int* duplist;
	int maxSize;
	int currentSize;
};

void processPath(char* path);
void checkPath(char* path);
void processSelf(char* arg);
int getFileSize(char* path, struct duplicate* dup);
int iterateDir(char* path, struct duplicate* dup);
int addDup(int inode, struct duplicate* dup);
int checkDup(int inode, struct duplicate* dup);
void extendDup(struct duplicate* dup, int newMaxSize);

int main(int argc, char* argv[]){
	if(argc == 1)
		processSelf(argv[0]);
	else if(argc == 2)
		processPath(argv[1]);
	else 
		printf("Too many arguments. Exiting\n");
	return 0;
}

//process the path given to start checking the size
void processPath(char* path){
	checkPath(path);
	struct duplicate* dup = (malloc(sizeof(struct duplicate)));
	dup->duplist = malloc(10 * sizeof(int));
	dup->maxSize = 10;
	dup->currentSize = 0;
	if(!dup){
		printf("error mallocing dup\n");
		exit(1);
	}
	printf("%i\t%s\n", iterateDir(path, dup), path);
	free(dup->duplist);
	free(dup);
}

//checks a path to see if its a directory
//exits if its an invalid path
void checkPath(char* path){
	if(!opendir(path)){
		printf("%s is an invalid path. Exiting.\n", path);
		exit(1);	
	}
}

//get the current working directory of arg
void processSelf(char* arg){
	char absolutePath[1024];
	if(!getcwd(absolutePath, 1024)){
		printf("Error obtaining the path for %s\n", arg);
		exit(1);
	}
	processPath(absolutePath);
}

//iterates through each directory recursively 
//in order to add up the total size of the directory
//passed to it
int iterateDir(char *path, struct duplicate* dup){
	int size = 0, foldersize = 0;
	DIR *d = opendir(path), *folder;
	struct dirent* dir;
	char* subpath;
	struct stat fileInfo;
	size = getFileSize(path, dup);
	while(d && ((dir = readdir(d)) != NULL)){
		if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
			continue;

		subpath = malloc(2 + sizeof(dir->d_name) + sizeof(path));
		if(!subpath){
			printf("error with malloc");
			exit(1);
		}

		strcpy(subpath, path);
		//check if the end of the path provided ends in /, and append one if not
		if(strcmp(path+strlen(path)-1, "/") != 0)
			strcat(subpath, "/");
		strcat(subpath, dir->d_name);
		lstat(subpath, &fileInfo);
		
		//open the directory if it isn't a symbolic link
		if((folder = opendir(subpath)) && !S_ISLNK(fileInfo.st_mode)){
			foldersize = iterateDir(subpath, dup);
			size = size + foldersize;
			printf("%i\t%s\n", foldersize, subpath);
			closedir(folder);
		}//get the disk usage of a symbolic link
		else if(S_ISLNK(fileInfo.st_mode)){
			size = size + fileInfo.st_blocks/2;
			continue;
		}
		size = size + getFileSize(subpath, dup);
		free(subpath);
	}
	closedir(d);
	return size;
}

//returns the disk usage of the given file. 
//returns 0 if the inode of file already exists
//in dup
int getFileSize(char* file, struct duplicate* dup){
	struct stat buf;
	if(lstat(file, &buf) == -1){
		printf("%s: error with lstat\n", file);
		return 0;
	}
	if(checkDup(buf.st_ino, dup))
		return 0;
	if(!addDup(buf.st_ino, dup)){
		printf("error adding to duplicate list\n");
		exit(1);
	}
	return buf.st_blocks/2;
}

//adds an inode to the duplicate list
//extends the list size by calling extendDup 
//if necessary. returns 1 on success 
int addDup(int inode, struct duplicate* dup){
	int lastpos = (dup)->currentSize;
	if(((dup)->currentSize) >= ((dup)->maxSize))
		extendDup(dup, ((dup)->maxSize) * 2);
	for(int i = 0; i < (dup)->currentSize; i++){
		if((dup)->duplist[i] == inode)
			return 0;
	}
	(dup)->duplist[lastpos] = inode;
	(dup)->currentSize++;
	return 1;
}

//returns 1 if the inode is found in the duplicate list
int checkDup(int inode, struct duplicate* dup){
	for(int i = 0; i < ((dup)->currentSize); i++){
		if((dup)->duplist[i] == inode)
			return 1;
	}
	return 0;
}

//extends the size of the duplicate list
void extendDup(struct duplicate* dup, int newMaxSize){
	if(newMaxSize < dup->maxSize){
		printf("new size is invalid");
		exit(1);
	}
	int* newduplist = realloc((dup)->duplist, newMaxSize * sizeof(int));
	if(!newduplist){
		printf("error reallocating memory\n");
		exit(1);
	}
	(dup)->duplist = newduplist;
	(dup)->maxSize = newMaxSize;
}
