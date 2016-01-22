/*
 * Connector of SI data SOCKET to printing to screen
 */

#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "si_base.h"
#include "si_print.h"

#define SOCKET_NAME "/tmp/si_data_sock"

int main(void){
	int sockfd, clifd;
	size_t clilen;
	struct sockaddr_un srv_addr, cli_addr;
    int r;
    struct s_sidata data;

	si_verbose = 1;		// set si library verbose level

	if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		error(EXIT_FAILURE, errno, "Cannot open socket.");
	}
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, SOCKET_NAME);
	unlink(srv_addr.sun_path);
    if(bind(sockfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0){
		error(EXIT_FAILURE, errno, "Cannot bind socket.");
	}
	listen(sockfd, 5);
	do {
		clilen = sizeof(cli_addr);
		clifd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if(clifd < 0){
			error(EXIT_SUCCESS, errno, "Cannot accept socket connection.");
		}else{
			if((r = read(clifd, &data, sizeof(data))) > 0){
				si_print_card(&data, stdout);
			}
		}
		printf(">>> Read bytes: %d\n", r);
		close(clifd);
	} while(1);
	close(sockfd);
    return 0;
}
    
