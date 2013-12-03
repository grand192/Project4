/*
* CSci4061 F2013 Assignment 4
* section: 3
* date: 12/02/13
* name: Devon Grandahl, Alex Cook
* id: 4260296, 4123940 
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
#define MAX_FILE_SIZE 100*4096 //400kb

//Structure for queue.
typedef struct request_queue
{
	int		m_socket;
	char	m_szRequest[MAX_REQUEST_LENGTH];
} request_queue_t;

request_queue_t Q[MAX_QUEUE_SIZE];
pthread_mutex_t q_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mtx = PTHREAD_MUTEX_INITIALIZER;
FILE *web_server_log; 


/* ///////////////////////////////////////
/////////QUEUE IMPLEMENTATION ///////////
/////////////////////////////////////////*/
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
			if(memcpy(Q[i].m_szRequest, request.m_szRequest,  MAX_REQUEST_LENGTH) == NULL){
				pthread_mutex_unlock(&q_mtx);
				return -1; //no open positions in queue
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
			request->m_socket = Q[i].m_socket;
			if(memcpy(request->m_szRequest, Q[i].m_szRequest,  MAX_REQUEST_LENGTH) == NULL){
				pthread_mutex_unlock(&q_mtx);
				printf("memcpy failed.\n");
				return 0;
			}	
			Q[i].m_socket = -1;
			pthread_mutex_unlock(&q_mtx);
			return 1; //shows we retrieved a request
		}
	}
	pthread_mutex_unlock(&q_mtx);
	return 0;
}
//////////////////////////////////////////////////
//////////////////////////////////////////////////

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
	strcpy(substr, file+(len-5));//not sure if this will work right
	if ( !strcmp(substr, ".html") || !strcmp(substr+1 , ".htm") )
		return 0;
	else if ( !strcmp( substr+1 , ".jpg") )
		return 1;
	else if ( !strcmp( substr+1 , ".gif") )
		return 2;
	else return 3;
}

// Worker thread function
void * worker(void * arg)
{
	request_queue_t job;
	char buf[MAX_FILE_SIZE];
	char contentType[50];
	ssize_t bytes_read;
	int num_jobs = 0;
	int fd; 
	int offset = 0;

	while(1){
		if( retrieve_request (&job)) {
			num_jobs++; //increment job count
			if (job.m_szRequest[0] == '/'){
				offset=1;
			}
			fd = open(job.m_szRequest + offset, O_RDONLY);
			if( (bytes_read = read(fd, buf, MAX_FILE_SIZE) ) == -1){
				pthread_mutex_lock(&log_mtx);
				if( (web_server_log = fopen("web_server_log","a") ) == NULL){
					perror("Failed to open file");
				}
				if(!fprintf(web_server_log,"[%u][%d][%d][%s][%d]\n", pthread_self(), num_jobs, job.m_socket, job.m_szRequest, "File could not be read." )){
					perror("fprintf failed: ");
				}
				fclose(web_server_log); 
				pthread_mutex_unlock(&log_mtx);
				return_error(job.m_socket, buf);
				perror("File could not be read.");
			}
			else {
				switch ( parseContentType(job.m_szRequest) )
				{
					case 0:
						strcpy(contentType, "text/html");
						break;
					case 1:
						strcpy(contentType, "image/jpeg");
						break;
					case 2:
						strcpy(contentType, "image/gif");
						break;
					case 3:
						strcpy(contentType, "text/plain");
						break;
					default:
						perror("retrieve_request failed miserably.");
				}
				pthread_mutex_lock(&log_mtx);
				if( (web_server_log = fopen("web_server_log","a") ) == NULL){
					perror("Failed to open file");
				}
				if(!fprintf(web_server_log,"[%u][%d][%d][%s][%d]\n", pthread_self(), num_jobs, job.m_socket, job.m_szRequest, bytes_read )){
					perror("fprintf failed: ");
				}
				fclose(web_server_log); 
				pthread_mutex_unlock(&log_mtx);
				close(fd);
				return_result(job.m_socket, contentType, buf, bytes_read);
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
	//Initialize the server with the port number passed in
	init(port);
	int num_total_threads = num_dispatcher + num_workers;
	int i;
	initialize(Q);
	
	web_server_log = fopen("web_server_log","w");
	if(fprintf(web_server_log, "" ) < 0){
		perror("failed to clear log: ");
	}
	fclose(web_server_log);

	// Create thread ID's starting from 1 to num_dispatcher
	for (i = 0; i < num_dispatcher; i++){
		pthread_t tid;
		pthread_create(&tid, NULL, &dispatch, NULL);
	}

	// Create unique thread ID's starting from num_dispatcher
	for (i = 0; i < num_workers; i++){
		pthread_t tid;
		pthread_create(&tid, NULL, &worker, NULL);

	}
	
	while(1);
	return 0;
}
