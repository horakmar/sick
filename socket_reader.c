/*
 * Reader of SI data writing to socket
 */

#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "si_base.h"

#define SOCKET_NAME "/tmp/si_data_sock"
#define READER_TICK_TIMER 60 			// seconds

void termination_handler(int signum){
	f_term = 1;
}

int main(void){
    int sockfd;
	struct sockaddr_un srv_addr;
	struct s_dev *first_dev, *dev, **pp_dev;

	si_verbose = 1;		// set si library verbose level

	if(si_detect_devices(&first_dev) == 0){
		error(EXIT_FAILURE, 0, "No SI devices detected.");
	}

    pp_dev = &first_dev;
	while(*pp_dev != NULL){
		if(((*pp_dev)->fd = si_initserial((*pp_dev)->devfile)) == -1){
			error(EXIT_SUCCESS, errno, "Cannot open device %s", (*pp_dev)->devfile);
            *pp_dev = (*pp_dev)->next; // Excluded from list, but NOT freed from memory
		}else if(! IS_NEW(si_station_detect((*pp_dev)->fd))){
            error(EXIT_SUCCESS, 0, "Old SI station (%s) is not supported.", (*pp_dev)->devfile);
            *pp_dev = (*pp_dev)->next; // Excluded from list, but NOT freed from memory
        }else if(((*pp_dev)->prot = si_station_setprot((*pp_dev)->fd)) == -1){
	    	error(EXIT_SUCCESS, 0, "Cannot setup SI station (%s) protocol.", (*pp_dev)->devfile);
            *pp_dev = (*pp_dev)->next; // Excluded from list, but NOT freed from memory
        }else{
            pp_dev = &((*pp_dev)->next);
        }
	}
	if(first_dev == NULL){
		error(EXIT_FAILURE, 0, "No devices can be opened.");
	}

	signal(SIGINT, termination_handler);
	signal(SIGQUIT, termination_handler);
	signal(SIGTERM, termination_handler);
	signal(SIGHUP, termination_handler);

	if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		error(EXIT_FAILURE, errno, "Cannot open socket.");
	}

	srv_addr.sun_family = AF_UNIX;
    strcpy(srv_addr.sun_path, SOCKET_NAME);
    if (connect(sockfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        error(EXIT_FAILURE, errno, "Cannot connect to server socket.");
    }
	si_reader_m(first_dev, sockfd, READER_TICK_TIMER);
	close(sockfd);

// SI device reset to original state procedure
    dev = first_dev;
    while(dev != NULL){
        if(si_station_resetprot(dev->fd, dev->prot) < 0){
            error(EXIT_SUCCESS, 0, "Reset SI station setup failed for %s.", dev->devfile);
        }
        dev = dev->next;
    }
    return 0;
}
    
