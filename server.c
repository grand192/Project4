/* csci4061 F2013 Assignment 4 
* section: one_digit_number 
* date: mm/dd/yy 
* names: Name of each member of the team (for partners)
* UMN Internet ID, Student ID (xxxxxxxx, 4444444), (for partners)
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "util.h"

#define MAX_THREADS 100
#define MAX_QUEUE_SIZE 100
#define MAX_REQUEST_LENGTH 1024

//Structure for queue.
typedef struct request_queue
{
	int		m_socket;
	char	m_szRequest[MAX_REQUEST_LENGTH];
} request_queue_t;

typedef struct Q
{
	int is_empty;
	request_queue_t queue[MAX_QUEUE_SIZE];
}

// Dispatcher thread function
void * dispatch(void * arg)
{
	request_queue_t request;
	while(1){
		request.m_socket = accept_connection(); 
		get_request(request.m_socket, request.m_szRequest); 
	}
}

int parseContentType(char* file){
	int len = strlen(file);
	char substr[5];
	strncpy(substr, file+(len-6), 5);//not sure if this will work right
	if ( strcmp(substr, ".html") || strcmp(substr+1 , ".htm")
		return 0;
	else if ( strcmp(substr+1 , ".jpg") )
		return 1;
	else if (strcmp(substr+1 , ".gif") )
		return 2;
	else return 3;
}

// Worker thread function
void * worker(void * arg)
{
	request_queue_t job;
	while(1){
		return_result(job.m_socket, "text/html", /*location of read in file*/, /*amount read in from file*/);
	}
}


int main(int argc, char **argv)
{
	//Error check first.
	if(argc != 6 && argc != 7)
	{
		printf("usage: %s port path num_dispatcher num_workers queue_length [cache_size]\n", argv[0]);
		return -1;
	} 

	// Take in the port number and check that it is valid
	int port = atoi(argv[1]);

	if (port <= 1025 || port >=  65535) {
		printf("Port number (the first argument to server) must be between 1025 and 65535\n");
		return -1;
	}

	// Set up configurations from passed in arguments
	char* path = argv[2];
	int num_dispatcher = atoi(argv[3]);
	int num_workers = atoi(argv[4]);
	int qlen = atoi(argv[5]);
	
	printf("Call init() first and make a dispather and worker threads\n");
	
	//Initialize the server with the port number passed in
	init(port);
	int num_total_threads = num_dispatcher + num_workers;
	int i;

	// Create thread ID's starting from 1 to num_dispatcher
	for (i = 1; i <= num_dispatcher; i++){
		pthread_t tid = i;
		pthread_create(&tid, NULL, &dispatch, NULL);
	}

	// Create unique thread ID's starting from num_dispatcher
	for (i = num_dispatcher+1; i <= num_total_threads; i++){
		pthread_t tid = i;
		pthread_create(&tid, NULL, &worker, NULL);
	}
	
	

	return 0;
}
