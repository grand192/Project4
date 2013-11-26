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
#define MAX_FILE_SIZE 4096 //4kb

//Structure for queue.
typedef struct request_queue
{
	int		m_socket;
	char	m_szRequest[MAX_REQUEST_LENGTH];
} request_queue_t;

request_queue_t Q[MAX_QUEUE_SIZE];
pthread_mutex_t q_mtx = PTHREAD_MUTEX_INITIALIZER;

void initialize (request_queue_t *Q){
	int i;
	pthread_mutex_lock(&q_mtx);
	for (i=0; i < MAX_QUEUE_SIZE; i++){
		Q[i].m_socket = -1; //empty sockets are -1
	}
	pthread_mutex_unlock(&q_mtx);
}

int isEmpty(request_queue_t *Q){
	int i;
	int empty;
	empty = 1;
	pthread_mutex_lock(&q_mtx);
	for (i = 0; i < MAX_QUEUE_SIZE; i++){
		if (Q[i].m_socket != -1){
			empty=0;
		}
	}	
	pthread_mutex_unlock(&q_mtx);
	return empty;
}

int addRequest(request_queue_t request){
	pthread_mutex_lock(&q_mtx);
	int i;
	for (i = 0; i < MAX_QUEUE_SIZE; i++){
		if (Q[i].m_socket == -1){
			Q[i].m_socket = request.m_socket;
			printf("Added a request\n");
			if(memcpy(Q[i].m_szRequest, request.m_szRequest,  MAX_REQUEST_LENGTH) == NULL){
				pthread_mutex_unlock(&q_mtx);
				return -1;
			}	
			break;
		}
	}
	pthread_mutex_unlock(&q_mtx);
	return 0;
}

int retrieve_request(request_queue_t *request){
	pthread_mutex_lock(&q_mtx);
	int i;
	for (i = 0; i < MAX_QUEUE_SIZE; i++){
		if (Q[i].m_socket != -1){
			printf("Retrieved a request\n");

			request->m_socket = Q[i].m_socket;
			if(memcpy(request->m_szRequest, Q[i].m_szRequest,  MAX_REQUEST_LENGTH) == NULL){
				pthread_mutex_unlock(&q_mtx);
				return -1;
			}	
			Q[i].m_socket = -1;
			return 1; //shows we retrieved a request
		}
	}
	pthread_mutex_unlock(&q_mtx);
	return 0;
}

// Dispatcher thread function
void * dispatch(void * arg)
{
	request_queue_t request;
	while(1){
		request.m_socket = accept_connection(); 
		get_request( request.m_socket, request.m_szRequest); 
		if ( addRequest(request) == -1){
			printf("The request could not be added by dispatcher of pthread_id: %d\n", pthread_self());
		}
	}
}

int parseContentType(char* file){
	int len = strlen(file);
	char substr[5];
	strncpy(substr, file+(len-6), 5);//not sure if this will work right
	if ( strcmp(substr, ".html") || strcmp(substr+1 , ".htm") )
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
	char buf[MAX_FILE_SIZE];
	int bytes_read;
	int num_jobs = 0;

	while(1){
		if( retrieve_request (&job)) {
			if( bytes_read = read(job.m_socket, buf, MAX_FILE_SIZE) == -1){
				perror("File could not be read.");
			}
			else {
				switch ( parseContentType(job.m_szRequest) )
				{
					case 0:
						return_result(job.m_socket, "text/html", buf, bytes_read);
						break;
					case 1:
						return_result(job.m_socket, "image/jpeg", buf, bytes_read);
						break;
					case 2:
						return_result(job.m_socket, "image/gif", buf, bytes_read);
						break;
					case 3:
						return_result(job.m_socket, "text/plain", buf, bytes_read);
						break;
					default:
						perror("retrieve_request failed miserably.");
				}
			num_jobs++; //increment job count
			}
		}
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
	
	if ( chdir(path) == -1){
		perror("Failed to change directories. Exit.");
		exit(-1);
	}

	printf("Call init() first and make a dispather and worker threads\n");
	
	//Initialize the server with the port number passed in
	init(port);
	int num_total_threads = num_dispatcher + num_workers;
	int i;
	initialize(Q);

	/*
	printf("Q's first value: %d\n", Q[0].m_socket );
	printf("Is it empty? : %d\n", isEmpty(Q));
	Q[0].m_socket = 5;
	printf("Is it empty? : %d\n", isEmpty(Q));
	*/


	// Create thread ID's starting from 1 to num_dispatcher
	for (i = 1; i <= num_dispatcher; i++){
		pthread_t tid;
		tid = i;
		pthread_create(&tid, NULL, &dispatch, NULL);
	}

	// Create unique thread ID's starting from num_dispatcher
	for (i = num_dispatcher+1; i <= num_total_threads; i++){
		pthread_t tid;
		tid = i;
		pthread_create(&tid, NULL, &worker, NULL);
	}
	
	while(1);

	return 0;
}
