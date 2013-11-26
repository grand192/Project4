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
#define MAX_FILE_SIZE 10*4096 //40kb

//Structure for queue.
typedef struct request_queue
{
	int		m_socket;
	char	m_szRequest[MAX_REQUEST_LENGTH];
} request_queue_t;

request_queue_t Q[MAX_QUEUE_SIZE];
pthread_mutex_t q_mtx = PTHREAD_MUTEX_INITIALIZER;
FILE *web_server_log; 



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
	printf("Request socket: %d\n", request.m_socket );

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
	printf("Request socket: %d\n", request.m_socket );

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
			printf("Retrieved socket: %d\n", request->m_socket );

			if(memcpy(request->m_szRequest, Q[i].m_szRequest,  MAX_REQUEST_LENGTH) == NULL){
				pthread_mutex_unlock(&q_mtx);
				printf("memcpy failed.\n");
				return 0;
			}	
			Q[i].m_socket = -1;
			printf("%s\n", request->m_szRequest);
			pthread_mutex_unlock(&q_mtx);
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
		printf("Request socket: %d\n", request.m_socket );
		get_request( request.m_socket, request.m_szRequest); 
		printf("The request was: %s \n", request.m_szRequest );
		if ( addRequest(request) == -1){
			printf("The request could not be added by dispatcher of pthread_id: %d\n", pthread_self());
		}
	}
}

int parseContentType(char* file){
	int len = strlen(file);
	char substr[5];
	strcpy(substr, file+(len-5));//not sure if this will work right
	printf("Substring: %s\n", substr);
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
			if (job.m_szRequest[0] == '/'){
				offset=1;
			}
			fd = open(job.m_szRequest + offset, O_RDONLY);
			printf("Read socket: %d\n", fd );
			if( (bytes_read = read(fd, buf, MAX_FILE_SIZE) ) == -1){
				if(!fprintf(web_server_log,"[%d][%d][%d][%s][%s]\n", 0, 0, job.m_socket, job.m_szRequest, "Error" )){
					perror("fprintf failed: ");
				}
				return_error(job.m_socket, buf);
				perror("File could not be read.");
			}
			else {
				printf("Retrieved request!\n");
				printf("Parse Content return: %d \n", parseContentType(job.m_szRequest));
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
				printf("%s\n",contentType );
				if(!fprintf(web_server_log,"[%d][%d][%d][%s][%d]\n", 0, 0, job.m_socket, job.m_szRequest, bytes_read )){
					perror("fprintf failed: ");
				}
				return_result(job.m_socket, contentType, buf, bytes_read);
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
	web_server_log = fopen("web_server_log","w"); 
	fprintf(web_server_log,"This is just an example :)"); 


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
	fclose(web_server_log); 
	return 0;
}
