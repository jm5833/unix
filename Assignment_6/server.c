#include <pthread.h>	//pthread functions
#include <netinet/in.h> //sockaddr_in
#include <unistd.h>		//open close
#include <errno.h>		//perror
#include <stdio.h>	
#include <stdlib.h>		//malloc, realloc, free
#include <stdlib.h>
#include <sys/select.h> //select
#include <sys/socket.h>	//socket
#include <arpa/inet.h>	//inet_addr
#include <string.h>		//memset

#define LISTEN_BACKLOG 15
#define DEFAULT_NUM_USERS 20
#define MESSAGE_LENGTH 1100
/*
	have a shared buffer
	lock the buffer when a producer wants to write to the buffer
	have the consumer "consume" the buffer and empty it out

*/


//struct to hold client information
struct clientinfo{
	int overwrite;		//Indicates if the clientinfo struct can be overwritten
						//due to a client disconnecting from the server
						//0 == cannot be overwritten, 1 == can be overwritten
	int sock;			//Socket number that the client is tied to
	char uname[100];	//Username of the client
						//enforcing a 100 char username limit
};						//in client.c 

//struct to hold a list of all clients
struct clientlist{
	struct clientinfo *client; //list of all clientinfos
	int maxSize;				//number of clients that is supported by the list
	int currentSize;			//currentsize indicates the number of connected users
	int hole;					//hole indicates which position in the clientlist 
};								//can be overwritten

void setupmutex();
void setupsock(int *sock, struct sockaddr_in *serveraddr, int port);
void setupclient();
void errorcheck(int retval, int errorval, char *function);
void pthread_check(int retval, char *function);
struct clientinfo* addClient(int socket, char username[100]);				
void removeClient(int socket);								
void *server(void *arg);
void *consumer();
void *producer(void *arg);

struct clientlist allclients;
char MESSAGE_BUFFER[MESSAGE_LENGTH];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clientMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condc = PTHREAD_COND_INITIALIZER;
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]){
	int retval, port;
	pthread_t ser, con;

	//setup the clientlist
	setupclient();
	//create the server thread
	port = 5555;
	if(argc >= 2)
		port = *argv[1];
	retval = pthread_create(&ser, NULL, &server, &port);
	pthread_check(retval, "pthread_create()");
	//create the consumer thread
	retval = pthread_create(&con, NULL, &consumer, NULL);
	pthread_check(retval, "pthread_create()");
	
	//create the server producer thread
	//Wait for the server and consumer to finish
	pthread_join(ser, NULL);
	
	return 0;
}

void setupmutex(){
	//initialize the mutex
	int retval = 0;
	//set up the mutex and error check the result
	retval = pthread_mutex_init(&mutex, NULL);
	pthread_check(retval, "pthread_mutex_init()");
	retval = pthread_mutex_init(&clientMutex, NULL);
	pthread_check(retval, "pthread_mutex_init()");
	//set up the condition variables and check the results
	retval = pthread_cond_init(&condc, NULL);
	pthread_check(retval, "pthread_cond_init()");
	retval = pthread_cond_init(&condp, NULL);
	pthread_check(retval, "pthread_cond_init()");

}
//setup the server socket to accept incomming connections
void setupsock(int *sock, struct sockaddr_in *serveraddr, int port){
	int retval = 0;
	//setup the socket address
	serveraddr->sin_family = AF_INET;
	serveraddr->sin_port = htons(port);
	retval = serveraddr->sin_addr.s_addr = inet_addr("127.0.0.1");
	errorcheck(retval, INADDR_NONE, "inet_addr()");
	//create the socket
	*sock = socket(AF_INET, SOCK_STREAM, 0);
	errorcheck(*sock, -1, "socket()");
	//bind the socket
	retval = bind(*sock, (struct sockaddr *) serveraddr, sizeof(struct sockaddr_in));
	errorcheck(retval, -1, "bind()");
	//start listening on the server socket
	retval = listen((*sock), LISTEN_BACKLOG);
	errorcheck(retval, -1, "listen()");
}

//setup a clientlist struct
void setupclient(){
	int retval = 0;
	//aquire the lock 
	retval = pthread_mutex_lock(&clientMutex);
	pthread_check(retval, "pthread_mutex_lock()");
	//initialize the clientinfo variable within allclients 
	//creating LISTEN_BACKLOG clients because thats what the backlog is being
	//defaulted to 
	allclients.client = malloc(LISTEN_BACKLOG * sizeof(struct clientinfo));
	allclients.maxSize = LISTEN_BACKLOG;
	allclients.currentSize = 0;
	allclients.hole = 0;
	for(int i = 0; i < LISTEN_BACKLOG; i++){
		allclients.client[i].sock = -1;
		allclients.client[i].overwrite = 1;
	}
	//release the lock
	retval = pthread_mutex_unlock(&clientMutex);
	pthread_check(retval, "pthread_mutex_unlock()");
	
}


//takes the int return value of a function and checks it against
//an error value. prints an error using perror if the function supports it
void errorcheck(int retval, int errorval, char *function){
	if(retval == errorval){
		perror(function);
		printf("Exit(): %s\n", function);
		exit(1);
	}
}

//error check function for pthread function calls
void pthread_check(int retval, char *function){
	if(retval != 0){
		perror(function);
		printf("exiting from pthread function: %s", function);
		exit(1);
	}
}

struct clientinfo* addClient(int socket, char username[100]){
	struct clientinfo* cptr;
	int retval, index;

	cptr = NULL;
	retval = 0;
	index = 0;

	//index is used as a substitue for allclients.hole, code  
	//gets messy with "allclients.hole" all over the place
	retval = pthread_mutex_lock(&clientMutex);
	pthread_check(retval, "pthread_mutex_lock()");
	index = allclients.hole;
	if(allclients.currentSize >= allclients.maxSize){
		allclients.maxSize = (allclients.maxSize + 1) * 2;
		allclients.client = realloc(allclients.client, allclients.maxSize * sizeof(struct clientlist));
		if(!allclients.client){
			printf("Realloc failed.\n");
			exit(1);
		}
		allclients.hole = allclients.currentSize;
		index = allclients.hole;
	}
	//inset client information into the clientinfo struct at index
	if(index != -1){
		allclients.client[index].sock = socket;
		strcpy(allclients.client[index].uname, username);
		allclients.client[index].overwrite = 0;
		allclients.currentSize++;
		cptr = &allclients.client[index];
	}
	//find the first clientinfo struct that has the overwrite int set
	for(int i = 1; i < allclients.maxSize; i++){
		allclients.hole = -1;
		if(allclients.client[i].overwrite == 1){
			allclients.hole = i;
			break;
		}
	}
	retval = pthread_mutex_unlock(&clientMutex);
	pthread_check(retval, "pthread_mutex_unlock()");
	printf("%s sucessfully added to allclients\n", username);
	return cptr;	
}

void removeClient(int socket){
	int retval;
	retval = pthread_mutex_lock(&clientMutex);
	pthread_check(retval, "pthread_mutex_lock()");

	for(int i = 0; i < allclients.maxSize; i++){
		if(allclients.client[i].sock != socket)
			continue;
		allclients.client[i].overwrite = 1;
		allclients.currentSize--;
		
	}
	allclients.hole = -1;
	for(int i = 0; i < allclients.maxSize; i++){
		if(allclients.client[i].overwrite == 0)
			continue;
		allclients.hole = i;
		break;
	}
	retval = pthread_mutex_unlock(&clientMutex);
	pthread_check(retval, "pthread_mutex_unlock()");
}
//function for the server thread, sets up the socket, mutex, and 
//infinitely loops while accepting connections to the server. 
//creates a thread each time a new client successfully connects
void *server(void *arg){
	int serverSocket, clientSocket, retval, port;
	struct sockaddr_in serveraddr;
	char username[100];
	pthread_t pro;

	port = *((int*)arg);
	retval = -1;

	//zeroing out the message queue to get 
	//rid of any garbage that may be in it
	memset(MESSAGE_BUFFER, 0, MESSAGE_LENGTH);
	//function call to set up the mutex
	setupmutex();
	//function call to set up the socket
	setupsock(&serverSocket, &serveraddr, port);

	while(1){
		retval = -1;
		clientSocket = accept(serverSocket, NULL, NULL);
		printf("Client Accepted. Checking the return value of accept(%i)\n", clientSocket);
		errorcheck(clientSocket, -1, "accept()");

		retval = read(clientSocket, username, 100);
		
		retval = pthread_mutex_lock(&mutex);
		pthread_check(retval, "pthread_mutex_lock()");
		while(strnlen(MESSAGE_BUFFER, MESSAGE_LENGTH) > 0){
			retval = pthread_cond_wait(&condc, &mutex);
			pthread_check(retval, "pthread_cond_wait()");
		}
		//zero out the buffer just to be sure
		memset(MESSAGE_BUFFER, 0, MESSAGE_LENGTH);
		//inform connected clients of the newly connected client
		strncpy(MESSAGE_BUFFER, username, strnlen(username, MESSAGE_LENGTH));
		strncat(MESSAGE_BUFFER, " connected.\n", 12);
		retval = pthread_cond_signal(&condc);
		pthread_check(retval, "pthread_cond_signal()");
		retval = pthread_mutex_unlock(&mutex);
		pthread_check(retval, "pthread_mutex_unlock()");

		//create a new thread for the newly connected client
		retval = pthread_create(&pro, NULL, &producer, addClient(clientSocket, username));
		pthread_check(retval, "pthread_create()");
		printf("%s connected\n", username);
	}


}

//Function meant to be used for the thread that
//'consumes' the MESSAGE_BUFFER by writing to all
//connected clients
void *consumer(){
	int retval = 0, len = 0;
	char message[MESSAGE_LENGTH];

	memset(message, 0, MESSAGE_LENGTH);
	while(1){

		retval = pthread_mutex_lock(&mutex);
		pthread_check(retval, "pthread_mutex_lock()");
		while(strnlen(MESSAGE_BUFFER, MESSAGE_LENGTH) == 0){
			retval = pthread_cond_wait(&condc, &mutex);
			pthread_check(retval, "pthread_cond_wait()");
		}
		len = strnlen(MESSAGE_BUFFER, MESSAGE_LENGTH);
		strncpy(message, MESSAGE_BUFFER, len);
		memset(MESSAGE_BUFFER, 0, MESSAGE_LENGTH);
		pthread_cond_signal(&condp);
		retval = pthread_mutex_unlock(&mutex);
		pthread_check(retval, "pthread_mutex_unlock()");

		retval = pthread_mutex_lock(&clientMutex);
		pthread_check(retval, "pthread_mutex_lock()");
		for(int i = 0; i < allclients.maxSize; i++){
			if(allclients.client[i].overwrite == 0)
				printf("%i\n", allclients.client[i].sock);
		}
		//ERROR PRONE CODE
		for(int i = 0; i < allclients.maxSize; i++){
			if(allclients.client[i].overwrite == 1)
				continue;
			printf("Attempting to write to %i\n", allclients.client[i].sock);
			retval = write(allclients.client[i].sock, message, len);
			errorcheck(retval, 0, "write()");
		}
		//ERROR PRONE CODE
		
		retval = pthread_mutex_unlock(&clientMutex);
		pthread_check(retval, "pthread_mutex_unlock()");
	}
}

//Function meant to be used for any thread that
//writes to the MESSAGE_BUFFER
void *producer(void *arg){
	int retval,nbytes;
	char message[MESSAGE_LENGTH];
	struct clientinfo *client;

	client = (struct clientinfo*)arg;
	memset(message, 0, MESSAGE_LENGTH);
	retval = -1;
	nbytes = 0;

	while(1){
		//take input from the client
		nbytes = read(client->sock, message, MESSAGE_LENGTH);
		errorcheck(nbytes, -1, "read()");
		printf("\nmessage read: %s", message);
		//lock the thread once input is available
		retval = pthread_mutex_lock(&mutex);
		pthread_check(retval, "pthread_mutex_lock()");
		while(strlen(MESSAGE_BUFFER) > 0){
			retval = pthread_cond_wait(&condp, &mutex);
			pthread_check(retval, "pthread_cond_wait()");
		}
		strncpy(MESSAGE_BUFFER, message, nbytes);
		//aquire the lock for the allclients object
		retval = pthread_mutex_lock(&clientMutex);
		pthread_check(retval, "pthread_mutex_lock()");
		//send a message indicating that the client has disconnected 
		if(nbytes == 0){
			printf("%s disconnected\n", client->uname);
			strncpy(MESSAGE_BUFFER, client->uname, strnlen(client->uname, MESSAGE_LENGTH));
			strncat(MESSAGE_BUFFER, " disconnected.\n", 15);
		} 
		//signal to the consumer there is something to 'consume'
		retval = pthread_cond_signal(&condc);
		pthread_check(retval, "pthread_cond_signal()");
		retval = pthread_mutex_unlock(&clientMutex);
		pthread_check(retval, "pthread_mutex_unlock()");
		retval = pthread_mutex_unlock(&mutex);
		pthread_check(retval, "pthread_mutex_unlock()");
		if(nbytes == 0){
			removeClient(client->sock);
			pthread_exit(0);
		}
	}	
}
