/* 
 * File Name: cenProxy.c
 * Author: Viet An Truong
 * Email: vietan124@gmail.com
 */

#include "cenProxyC.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <netdb.h>
#include <assert.h>
#include <iostream>
#include <vector>

// global
#ifndef	_GNU_SOURCE
#define _GNU_SOURCE
#endif

#define MAX_BUFFER_LENGTH 8192
#define SERVERPORTNUM 12401

#define ERROR "GET http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/error.html HTTP/1.1\r\nHost: pages.cpsc.ucalgary.ca\r\n\r\n"

#define MAX_NUMBER_KEYWORDS 5

//#define DEBUG

int childsockfd;

int
main(int argc, char *argv[]) {
	struct sockaddr_in server, client;
	static struct sigaction act;
	char messagein[MAX_BUFFER_LENGTH];
	char messageout[MAX_BUFFER_LENGTH]; 

	char buffer[MAX_BUFFER_LENGTH];
	int parentsockfd, c, pid, conns;

	// signal handling
	act.sa_handler = catcher;
	sigfillset(&(act.sa_mask));
	sigaction(SIGPIPE, &act, NULL);

	/* Intialize server sockaddr structure */
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVERPORTNUM);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	/* set up the transport-level end point to use TCP */
	if((parentsockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "testserver: socket() call failed!\n");
		exit(1);
	}

	/* bind a specific address and port to the end point */
	if(bind(parentsockfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in) ) == -1) {
		fprintf(stderr, "testserver: bind() call failed\n");
		exit(1);
	}

	/* start listening for incoming connections from clients */
	if(listen(parentsockfd, 5) == -1) {
		fprintf(stderr, "testserver: listen() call failed\n");
		exit(1);
	}

	/* initialize message strings just to be safwe (null-terminated) */
	bzero(messagein, MAX_BUFFER_LENGTH);
	bzero(messageout, MAX_BUFFER_LENGTH);

	fprintf(stderr, "Welcome! Type HELP to receive list of commands!\n");
	fprintf(stderr, "Server listening on TCP port %d... \n\n", SERVERPORTNUM);

	conns = 0;
	c = sizeof(struct sockaddr_in);

	while (1) {
	  	// accept a connection
	  	if ((childsockfd = accept(parentsockfd, (struct sockaddr*)&client, (socklen_t *)&c)) == -1) {
	  	    fprintf(stderr, "testserver: accept() call failed!\n");
	  	    exit(1);
	  	}

	  	/* increment server's counter variable */
	
		conns++;
	
		/* try to create a child process to deal with this new client */
		  	
		pid = fork();
	
		/* use process if (pid) returned by fork to decide what to do next */
	
	  	if (pid < 0) {
	    	fprintf(stderr, "testserver: fork() call failed!\n");
	    	exit(1);
	  	} 
	  	else if (pid == 0) {
	    	close(parentsockfd);
	    	/* obtain message from this client */

	    	int bytes;
	    	int done = 0;
	    	int bufferLength;

	    	while (!done) {
			    bytes = 0;
			    while(1){
	      			bufferLength = recv(childsockfd, buffer, MAX_BUFFER_LENGTH, 0);
	      			bytes += bufferLength;

	      			strcat(messagein, buffer);

	      			/* Detect end of HTTP Request with double cartridge returns*/
	      			if (strstr(messagein, "\r\n\r\n") != NULL)	{
	      				break;	      			
	      			}
	  	  		}

#ifdef DEBUG
	      		printf("Child process received %d bytes with command:\n %s\n",
		     		bytes, messagein);
#endif

          		if(strncmp(messagein, "GET", 3) == 0) {
            		/* Parse the GET request to determine if the request is allowed */
          			if (argc > 0) { // if there are keywords to be blocked
	            		for (int i = 0; i < argc; i++) {
	                		if (strcasestr(messagein, argv[i]) != NULL) {
	                    		// modify the requesting URL, change it to error page
	                			std::cout << "Detected blocked keyword\n";	                	
	                			strcpy(messagein, ERROR);
	                			break;
	                		}
	            		}
        			}
            		/* parse messagein to get host name, and url */
            		char* hostStart = strstr(messagein, "Host");

            		assert(hostStart != NULL);  
            		hostStart += 6; // forward to the start of the actual URL
		
		            char* msg = new char[strlen(messagein)];
		            strcpy(msg, messagein);
		
		            char* host = strtok(hostStart, "\r"); // grab URL token
		            		
		            std::string res(forward(host, msg));
		
            		char *mesPtr = &res[0];
    	  			size_t mesLen = res.size();
    	  			ssize_t bytesSent;
    	  			ssize_t totalSizeSent = 0;
    	  			while (mesLen > 0 && (bytesSent = send(childsockfd, mesPtr, MAX_BUFFER_LENGTH, 0)) > 0) {
    					mesPtr += bytesSent;
    					mesLen -= (size_t)bytesSent;
    					totalSizeSent += bytesSent;
    	  			}
    	  			if (/*mesLen > 0 ||*/ bytesSent < 0) {
        				fprintf(stderr, "Send failed on connection %d\n", conns);
    		  		}
	
    	  			std::cout << "Total bytes sent to browser is: " << totalSizeSent << std::endl;
          		}
	      		else {
	      			sprintf(messageout, "INVALID, ENTER \"HELP\" FOR LIST OF VALID COMMANDS\n");
	      		}

#ifdef DEBUG
	      		printf("Child about to send message: %s\n", messageout);
#endif	      

	      		/* send the result message back to the client */

	      		if (*messageout != '\0') {
	      			char *mesPtr = messageout;

    	  			ssize_t bytesSent;
    	  			ssize_t totalSizeSent = 0;
    	  			bytesSent = send(childsockfd, mesPtr, MAX_BUFFER_LENGTH, 0);
    				if (bytesSent < 0) {
        				fprintf(stderr, "Send failed on connection %d\n", conns);
    				}
    	  		}

	      		bzero(messagein, MAX_BUFFER_LENGTH);
	     		bzero(messageout, MAX_BUFFER_LENGTH);
	    	}
	  	
	  	} else {
	  		/* the parent process is the one doing this part */

	  		fprintf(stderr, "Created child process %d to handle that client\n", pid);
	  		fprintf(stderr, "Parent going back to job of listening...\n\n");

	  		close (childsockfd);
	  	}
	}
}

std::string
forward(const char* host, char* request) {
	int socket_desc;
	struct sockaddr_in server;

	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_desc == -1){
        puts("could not create socket");
        return NULL;
    }
    puts("Socket created");
    server.sin_addr.s_addr = inet_addr(getIP(host));
    server.sin_family = AF_INET;
    server.sin_port = htons (80); // ?

    if (connect(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0){
        puts("connect error");
        return NULL;
    }

    puts("connected");

    if(sendRequest(socket_desc, request) == -1) {
    	puts("send failed");
    	return NULL;
    }

    puts("Request sent\n");

    return receiveResponse(socket_desc);
}

int
sendRequest(const int socket_desc, char* request) {
	char* ptr = request;
	size_t toSend = strlen(request); // bytes left to sent
	ssize_t bytesSent;

	// send in a loop to ensure all 
	while (toSend > 0 && (bytesSent = send(socket_desc, ptr, toSend, 0)) > 0) {
		ptr += bytesSent;
		toSend -= (size_t)bytesSent;
	}

	if (toSend > 0 || bytesSent < 0) {
		return -1; // return -1 when send failed
	}

	return 1;
}

std::string
receiveResponse (const int socket_desc){
	char buff[MAX_BUFFER_LENGTH];
	std::string response;
	ssize_t bytesReceived;
	ssize_t totalSize = 0;

	while ((bytesReceived = recv(socket_desc, buff, MAX_BUFFER_LENGTH -1, 0)) > 0) {
		response.append(buff, bytesReceived);
		// clear buffer for next iteration (just to make sure)
		bzero(buff, MAX_BUFFER_LENGTH);
		totalSize += bytesReceived;
	}

	if(bytesReceived < 0) {
		return 0;
	}

	std::cout << "total size of server response is: " << response.length() << std::endl;

	return response;
}

void
catcher(int sig){
	close(childsockfd);
	exit(0);
}

char*
getIP (const char* hostname){
	char ip[100];
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    //printf("hostname is: %s", hostname);
    if((he = gethostbyname (hostname)) == NULL){
        herror("gethostbyname");
        return NULL;
    }

    addr_list = (struct in_addr**) he->h_addr_list;

    return inet_ntoa(*addr_list[0]);
}