#include <stdio.h>
#include <string.h>
#include <stdlib.h>			//exit
#include <sys/socket.h>
#include <netinet/in.h>		//sockaddr_in
#include <arpa/inet.h>		//inet_addr
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <strings.h>

#define DEFAULT_PORT 5555
#define MESSAGE_LENGTH 1000

void setupSocket(int *serverSocket, struct sockaddr_in *server, int argc, char *argv[]);
void recieveMessage(int serverSocket);
void sendMessage(int serverSocket, char* username);

//./client username ip socket
int main(int argc, char *argv[]){
	int serverSocket;
	struct sockaddr_in server;
	fd_set readfd;		

	if(argc == 1){
		printf("Not enough arguments.\n");
		exit(1);
	}
	if(strlen(argv[1]) > 99){
		printf("Username to long. Please use a username thats less than 100 characters.\n");
		exit(1);
	}
	
	printf("Welcome %s. Type \"exit\" to terminate the program\n", argv[1]);

	setupSocket(&serverSocket, &server, argc, argv);
	while(1){
		FD_ZERO(&readfd);
		FD_SET(serverSocket, &readfd);
		FD_SET(STDIN_FILENO, &readfd);

		if(pselect(serverSocket + 1, &readfd, NULL, NULL, NULL, NULL) == -1){
			perror("pselect()");
			exit(1);
		}
		//work on a given fd
		if(FD_ISSET(serverSocket, &readfd))
			recieveMessage(serverSocket);
		if(FD_ISSET(STDIN_FILENO, &readfd))
			sendMessage(serverSocket, argv[1]);
	}
	return 0;
}

//sets up the client socket to connect to a server socket
void setupSocket(int *serverSocket, struct sockaddr_in *server, int argc, char *argv[]){
	int retval = 0;
	//socket creation
	(*serverSocket) = socket(AF_INET, SOCK_STREAM, 0);
	if((*serverSocket) == -1){
		perror("socket()");
		exit(1);
	}
	//set up the defaults for the socket to connect to
	(*server).sin_family = AF_INET;
	(*server).sin_port = htons(DEFAULT_PORT);
	retval = inet_pton(AF_INET, "127.0.0.1", &(server->sin_addr));
	//modify the parameters to connect to with any data the user gave at the command line
	if(retval == -1){
		perror("inet_pton()");
		exit(1);
	}
	if(argc > 2){
		retval = 0;
		retval = inet_pton(AF_INET, argv[2], &(server->sin_addr));
		if(retval == -1){
			perror("inet_pton()");
			exit(1);
		}
	}
	if(argc > 3)
		(*server).sin_port = htons((int)strtol(argv[3], NULL, 10));

	if(connect((*serverSocket), (struct sockaddr *) server, sizeof(*server)) == -1){
		perror("connect()");
		exit(1);
	}
	//99 + 1 because 99 chars for the username and the null terminating byte at the end
	write(*serverSocket, argv[1], strnlen(argv[1], 99) + 1);

}
//have the client recieve a message
void recieveMessage(int serverSocket){
	char message[MESSAGE_LENGTH];
	int retval;
	
	//clear out the message buffer to get rid of garbage data
	memset(message, 0, MESSAGE_LENGTH);
	retval = read(serverSocket, message, MESSAGE_LENGTH);
	if(retval < 1){
		perror("read()");
		shutdown(serverSocket, SHUT_RDWR);
		exit(1);
	}
	if(strlen(message) > 0)
		printf("%s", message);
}
//send a message to the server socket from the user
void sendMessage(int serverSocket, char* username){
	char message[MESSAGE_LENGTH], fullmessage[MESSAGE_LENGTH], *middle;
	int retval, nbytes;
	//clear out the message buffer to get rid of garbage dataa
	memset(message, 0, MESSAGE_LENGTH);
	memset(fullmessage, 0, MESSAGE_LENGTH);
	middle = ":";	
	
	if(fgets(message, MESSAGE_LENGTH, stdin) == NULL){
		perror("fgets");
		return;
	}
	if(strncasecmp(message, "exit\n", 5) == 0){
		printf("Disconnecting.\n");
		exit(0);
	}
	//append the username to the beginning of the message
	snprintf(fullmessage, strlen(username) + 1, username);
	snprintf(fullmessage + strlen(fullmessage), 2, middle);
	snprintf(fullmessage + strlen(fullmessage), MESSAGE_LENGTH - strlen(fullmessage) - 1, message);

	nbytes = strnlen(fullmessage, MESSAGE_LENGTH);
	retval = write(serverSocket, fullmessage, nbytes);
	
	//exit if there is an error with write
	if(retval < 1){
		perror("write()");
		shutdown(serverSocket, SHUT_RDWR);
		exit(1);
	} 
	else if(retval == 0){
		perror("nothing written");
	}
}
