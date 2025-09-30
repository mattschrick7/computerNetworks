#include "proxy_parse.h"
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
#include <netinet/tcp.h>

#define QUEUE_LENGTH 10
#define RECV_BUFFER_SIZE 1024

int connect_to_server(const char *host, int port) {
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	int target_fd;
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(host, port, &hints, &targetinfo)) != 0) {
		perror("getaadrinfo");
		return -1;
	}

	for (p = targetinfo; p != NULL; p = p->ai_next) {
        if ((target_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(target_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(target_fd);
            perror("client: connect");
            continue;
        }
        break; 
    	}

    	freeaddrinfo(targetinfo);

    	if (p == NULL) {
        	fprintf(stderr, "client: failed to connect to target\n");
        	return -1;
    	}

    	return target_fd; 

}

int parse_request(char *buffer, ssize_t len, char **host, char **port, char **request, size_t *request_len) {
	struct ParsedRequest *req = ParsedRequest_create();
	if (ParsedRequest(req, buffer, len) < 0) {
		fprintf(stderr, "parse failed\n");
		ParsedRequest_destroy(req);
		return -1;
	}

	if (strcmp(req->method, "GET") != 0) {
		fprintf(std, "Only GET requests are supportedf \n");
		ParsedRequest_destroy(req);
		return -1;
	}

	*host = strdup(req->host);
	*port = req->port ? strdup(req->port) : strdup(DEFAULT_HTTP_PORT);

	*request_len = ParsedRequest_totalLen(req);
	*request = (char*)malloc(*request_len + 1);
	if (ParsedRequest_unparse(req, *request, *request_len) < 0) {
        	fprintf(stderr, "unparse failed\n");
        	free(*host);
        	free(*port);
        	free(*request);
        	ParsedRequest_destroy(req);
        	return -1;
    	}

	ParsedRequest_destroy(req);
	return 0;
}

int receive_request(int client_fd, char **buffer, size_t *total_len) {
	char temp_buffer[RECV_BUFFER_SIZE];
	size_t buffer_size = RECV_BUFFER_SIZE;
	*buffer =(char *)malloc(buffer_size);

	if (!*buffer) {
		fprintf(stderr, "Failed to allocate buffer");
		return -1;
	}

	*total_len = 0;

	while (1) {
		ssize_t bytes_received = recv(client_fd, temp_buffer, RECV_BUFFER_SIZE, 0);
		if (bytes_received <= 0) {
			perror("error");
			free(*buffer);
			return -1;
		}
		if (*total_len + bytes_received >= buffer_size) {
			buffer_size = buffer_size * 2;
			char *new_buffer = (char *)realloc(*buffer, buffer_size);
			if (!new_buffer) {
				fprintf(stderr, "Failed to realloc buffer");
				free(*buffer);
				return -1;
			}
			*buffer = new_buffer;
		}

		memcpy(*buffer + *total_len, temp_buffer, bytes_received);
		*total_len += bytes_received;

		if (*total_len >= 4 && memcmp(*buffer + *total_len -4, "\r\n\r\n", 4) == 0) {
			break;
		}
	}

	return 0;
}


/* TODO: proxy()
 * Establish a socket connection to listen for incoming connections.
 * Accept each client request in a new process.
 * Parse header of request and get requested URL.
 * Get data from requested remote server.
 * Send data to the client
 * Return 0 on success, non-zero on failure
*/
int proxy(char *proxy_port) {
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

	  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		  perror("setsockopts");
		  exit(1);
	  }
	  if (bind(sockfd, p->ai_addr, p->addrlen) == -1) {
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

  while(1) {

	    while (waitpid(-1, NULL, WNOHANG) > 0) {
            	// Continue reaping until no more children are ready
            }
            sin_size = sizeof their_addr;
            new_fd = accept(sockfd, (struct sockaddr *)&their_addr,&sin_size);
            if (new_fd == -1) {
                        perror("accept");
                        continue;
            }

	    pid_t pid = fork();
	    if (pid == -1) {
		    perror("fork");
		    close(new_fd);
		    continue;
	    }
	
	    if (pid == 0) {
		/*
            	char msg[RECV_BUFFER_SIZE];
            	ssize_t bytes_received = 0;
            	ssize_t expected_size = RECV_BUFFER_SIZE;

            	while ((bytes_received = recv(new_fd, msg, RECV_BUFFER_SIZE, 0)) > 0) {
                    	fwrite(msg, bytes_received, 1, stdout);
                    	fflush(stdout);
			
		    	if (send(new_fd, msg, bytes_received, 0) == -1) {
				perror("send");
		    	}
            	}
            	if (bytes_received == -1) {
                	perror("recv");
            	}
            	close(new_fd);
		exit(0);

	    } else {
		    close(new_fd);
	    }
	    */
		    close(sockfd);
	    	    char *buffer = NULL;
		    size_t total_len = 0;
		    if (receive_request(new_fd, &buffer, &total_len) < 0) {
			    close(new_fd);
			    exit(1);
		    }

		    char *host = NULL, *port = NULL, *request = NULL;
		    size_t request_len = 0;
		    if (parse_request(buffer, total_len, &host, &port, &request, &request_len)<0) {
			    printf(stderr, "Failed to parse request\n");
                	    free(buffer);
                            close(new_fd);
                	    exit(1);
            	    
		    }

		    free(buffer);

		    target_fd = connect_to_server(host, port);
		    send(target_fd, request, request_len, 0);
		    free(request);

		    char response[RECV_BUFFER_SIZE];
		    ssize_t bytes_received = 0;
		    while ((bytes_received = recv(target_fd, msg, RECV_BUFFER_SIZE,0)) >0) {
			    if (send(new_fd, msg, bytes_received, 0) == -1) {
				    perror("send to client");
				    break;
			    }
		    }
		    if (bytes_received == -1) {
			    perror("recv from target");
		    }

		    free(host);
		    free(port);
		    close(new_fd);
		    close(target_fd);
		    exit(0);
  		

	    } else {
		    //parent 
		    close(new_fd);
	    }

  	}

  close(sockfd);
  return 0;

}


int main(int argc, char * argv[]) {
  char *proxy_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./proxy <port>\n");
    exit(EXIT_FAILURE);
  }

  proxy_port = argv[1];
  return proxy(proxy_port);
}
