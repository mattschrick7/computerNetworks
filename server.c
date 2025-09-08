#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

#define QUEUE_LENGTH 10
#define RECV_BUFFER_SIZE 2048

/* TODO: server()
 * Open socket and wait for client to connect
 * Print received message to stdout
 * Return 0 on success, non-zero on failure
*/
int server(char *server_port) {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
	
    int rv;
    int yes = 1;
    if ((rv = getaddrinfo(NULL, server_port, &hints, &servinfo)) != 0) {
	    perror("addrinfo failed");
    	    return 1;
    }
	
    for (p = servinfo; p != NULL; p = p->ai_next) {
	    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
		    perror("server:socket");
		    continue;
	    }

	    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1) {
		    perror("setsockopts");
		    exit(1);
	    }

	    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
		    close(sockfd);
		    perror("server:bind");
		    continue;
	    }
	    break;

    }
	
    freeaddrinfo(servinfo);
    if (listen(sockfd, QUEUE_LENGTH) == -1) {
	    perror("listen");
	    exit(1);
    }
    //main accept loop
    while(1) {
	    sin_size = sizeof their_addr;
	    new_fd = accept(sockfd, (struct sockaddr *)&their_addr,&sin_size);
	    if (new_fd == -1) {
		    	perror("accept");
			continue;
	    }
		
	    char msg[RECV_BUFFER_SIZE];
	    ssize_t bytes_received = 0;
	    ssize_t expected_size = RECV_BUFFER_SIZE;

	    while ((bytes_received = recv(new_fd, msg, sizeof(msg) -1, 0)) > 0) {
		    msg[bytes_received] = '\0'; //null terminate
		    printf(msg);

	    }
	    if (bytes_received == -1) {
		    perror("recv");
	    }
	    close(new_fd);
    }

    close(sockfd);
    return 0;    


}

/*
 * main():
 * Parse command-line arguments and call server function
*/
int main(int argc, char **argv) {
  char *server_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./server-c [server port]\n");
    exit(EXIT_FAILURE);
  }

  server_port = argv[1];
  return server(server_port);
}
