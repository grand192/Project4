/*
* CSci4061 F2013 Assignment 4
* section: 3
* date: 12/02/13
* name: Devon Grandahl, Alex Cook
* id: 4260296, 4123940 
*/

1. Run 'make' in the terminal from the directory that contains the c file

2. After running the 'make' command, the server can be executed by running the command 'web_server_http [port] [path] [num_dispatchers] [num_workers] [queue_length]', replacing the bracketed arguments with the desired values. An example of this is './web_server_http 9000 /home/grand192/4061/Project4 100 100 100 100'.

3. Our program is a multi-threaded web browser. Once started, it instantiates a thread for each worker and a thread for each dispatcher and waits for user input. When the user makes a request, it is caught by a dispatcher thread and put on a queue for the next available worker. Once a worker becomes available, it pulls a request off the queue, opens the requested file, reads the contents, and uses the return_result method to serve the request. Logging is also implemented so that each worker prints '[ThreadID#][Request#][fd][Request string][bytes/error]' into a file called 'web_server_log' each time the worker serves a request.

4. We are assuming no file will be larger than 400k, that the maximum number of threads will be 100 (of each type) and the maximum queue size will be 100.
