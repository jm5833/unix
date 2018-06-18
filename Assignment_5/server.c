#include <sys/select.h>		//pselect
#include <arpa/inet.h> 		//inet_addr
#include <sys/socket.h>		
#include <sys/un.h>
#include <sys/socket.h>
#include <errno.h>			//perror
#include <stdlib.h>			//exit
#include <stdio.h>			
#include <string.h>
#include <netinet/in.h>		//sockaddr_in
#include <unistd.h>			//close, write
#include <strings.h>
#define LISTEN_BACKLOG 15
#define DEFAULT_PORT 5555
#define MESSAGE_LENGTH 1000

void setupSocket(int *serverSocket, struct sockaddr_in *serveraddr, int argc, char *argv[]);
int recieveMessage(int clientSocket);
int sendMessage(int clientSocket,  char *username);
void selectinputs(int clientSocket, char *username);

int main(int argc, char* argv[]){
	int serverSocket, clientSocket;
	socklen_t clientlen;
	struct sockaddr_in serverAddr;
	struct sockaddr_storage clientStorage;
	
	if(argc == 1){
		printf("not enough arguments\n");
		exit(1);
	}
	setupSocket(&serverSocket, &serverAddr, argc, argv);
	
	while(1){
		printf("\nWaiting on clinet connections...\n");
		clientlen = sizeof(clientStorage);
		clientSocket = accept(serverSocket, (struct sockaddr *) &clientStorage, &clientlen);
		if(clientSocket == -1){
			perror("accept()");
			exit(1);
		}
		printf("Client connected\n");
		selectinputs(clientSocket, argv[1]);
	}
	close(serverSocket);
	printf("Exiting main\n");
	return 0;
}
//select an input to work on
void selectinputs(int clientSocket, char *username){
	fd_set readfd;
	int retval = 0;
	
	while(1){

		FD_ZERO(&readfd);
		FD_SET(clientSocket, &readfd);
		FD_SET(STDIN_FILENO, &readfd);
		fflush(stdout);
		if(pselect(clientSocket + 1, &readfd, NULL, NULL, NULL, NULL) == -1){
			perror("pselect()");
			exit(1);
		}

		//work on a given fd
		if(FD_ISSET(clientSocket, &readfd))
			retval = recieveMessage(clientSocket);
		else if(FD_ISSET(STDIN_FILENO, &readfd))
			retval = sendMessage(clientSocket, username);
		if(retval < 1)
			return;
	}
}
//set up the server socket to accept incoming connections
void setupSocket(int *serverSocket, struct sockaddr_in *serverAddr, int argc, char *argv[]){
	//set up the socket address
	(*serverAddr).sin_family = AF_INET;
	(*serverAddr).sin_port = htons(DEFAULT_PORT);
	if(argc > 2)
		(*serverAddr).sin_port = htons(strtol(argv[2], NULL, 10));
	//create the socket
	(*serverSocket) = socket(AF_INET, SOCK_STREAM, 0);
	if((*serverSocket) == -1){
		perror("socket()");
		exit(1);
	}
	//bind the socket
	if(bind((*serverSocket), (struct sockaddr *) serverAddr, sizeof(struct sockaddr_in)) == -1){
		perror("bind()");
		exit(1);
	}
	//start listening on the server socket
	if(listen((*serverSocket), LISTEN_BACKLOG) == -1){
		perror("listen()");
		exit(1);
	}
}
//recieves and displays a message from a client socket
int recieveMessage(int clientSocket){
	char message[MESSAGE_LENGTH];
	int retval;

	memset(message, 0, MESSAGE_LENGTH);
	retval = read(clientSocket, message, MESSAGE_LENGTH);
	if(retval < 1){
		perror("read()");
		shutdown(clientSocket, SHUT_RDWR);
	}
	if(strnlen(message, MESSAGE_LENGTH) > 0)
		printf("%s", message);
	return retval;
}
//prepares and sends a message to a client socket
int sendMessage(int clientSocket, char *username){
	char message[MESSAGE_LENGTH], fullmessage[MESSAGE_LENGTH], *middle;
	int retval, nbytes;
	
	//clear out the message buffer to get rid of garbage data
	memset(message, 0, MESSAGE_LENGTH);
	middle = ":";

	if(fgets(message, MESSAGE_LENGTH, stdin) == NULL){
		perror("fgets");
		return -1;
	}
	if(strncasecmp(message, "exit", 4) == 0){
		printf("Shutting server down.\n");
		shutdown(clientSocket, SHUT_RDWR);
		exit(1);
	}
	//append the username to the beginning of the message
	snprintf(fullmessage, strlen(username) + 1, username);
	snprintf(fullmessage + strlen(fullmessage), 2, middle);
	snprintf(fullmessage + strlen(fullmessage), MESSAGE_LENGTH - strlen(fullmessage) - 1, message);


	nbytes = strnlen(fullmessage, MESSAGE_LENGTH);
	retval = write(clientSocket, fullmessage, nbytes);
	if(retval == -1){
		perror("write()");
		shutdown(clientSocket, SHUT_RDWR);
	}
	return retval;
}
