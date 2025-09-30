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

#define SEND_BUFFER_SIZE 2048


/* TODO: client()
 * Open socket and send message from stdin.
 * Return 0 on success, non-zero on failure
*/
int client(char *server_ip, char *server_port) {
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *p;
	char ipstr[INET6_ADDRSTRLEN];
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	status = getaddrinfo(server_ip, server_port, &hints, &servinfo);
	//I believe servinfo now holds possible IP addresses for the server
	if (status != 0) {
		perror("Ip not found");
	}
	int socketfd;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		
		socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (socketfd == -1) {
			perror("socket error");
			continue;
		}
		if (connect(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("Connection Error");
			close(socketfd);
			continue;
		}
		break;
	}
	if (!socketfd){
		perror("couldn't connect to socket");
		return -1;
	}
	size_t bytesRead;
	char buf[SEND_BUFFER_SIZE];
	while((bytesRead = fread(buf, 1, SEND_BUFFER_SIZE, stdin)) > 0) {

		send(socketfd, buf, bytesRead, 0);
	} 
	freeaddrinfo(servinfo);
	close(socketfd);	
	return 0;
}

/*
 * main()
 * Parse command-line arguments and call client function
*/
int main(int argc, char **argv) {
  char *server_ip;
  char *server_port;

  if (argc != 3) {
    fprintf(stderr, "Usage: ./client-c [server IP] [server port] < [message]\n");
    exit(EXIT_FAILURE);
  }

  server_ip = argv[1];
  server_port = argv[2];
  return client(server_ip, server_port);
}
