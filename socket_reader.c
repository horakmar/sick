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

struct s_client {
	pid_t pid;
	struct s_client *next;
};

void termination_handler(int signum){
	f_term = 1;
}

int main(void){
    int sockfd;
	int c;
	pid_t pid;
	struct sockaddr_un srv_addr;
	struct s_dev *first_dev, *dev;
	struct s_client *first_cli, *cli, *newcli;

	si_verbose = 1;		// set si library verbose level

	if(si_detect_devices(&first_dev) == 0){
		error(EXIT_FAILURE, 0, "No SI devices detected.");
	}

	cli = NULL;
    dev = first_dev;
	while(dev != NULL){
		pid = fork();
		switch(pid){
		case -1:
			error(EXIT_FAILURE, errno, "Cannot create new process.\n");
		case 0: // Child - SI reader
			do {
				if((dev->fd = si_initserial(dev->devfile)) == -1){
					error(EXIT_FAILURE, errno, "Cannot open device %s", dev->devfile);
				}
				if(! IS_NEW(si_station_detect(dev->fd))){
					error(EXIT_SUCCESS, ERR_OLDSTATION, "Old SI station (%s) is not supported.", dev->devfile);
					close(dev->fd);
					exit(ERR_OLDSTATION);
				}
				if((dev->prot = si_station_setprot(dev->fd)) == -1){
					error(EXIT_SUCCESS, ERR_SETPROT, "Cannot setup SI station (%s) protocol.", dev->devfile);
					break;
				}

				signal(SIGINT, termination_handler);
				signal(SIGQUIT, termination_handler);
				signal(SIGTERM, termination_handler);
				signal(SIGHUP, termination_handler);
				
				while(f_term == 0){
					if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
						error(EXIT_SUCCESS, errno, "Cannot open socket.");
						break;
					}
					srv_addr.sun_family = AF_UNIX;
					strcpy(srv_addr.sun_path, SOCKET_NAME);
					if (connect(sockfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
						error(EXIT_FAILURE, errno, "Cannot connect to server socket.");
					}
					si_reader_s(dev->fd, sockfd, READER_TICK_TIMER);
					close(sockfd);
				}
			} while(0);
				
// SI device reset to original state procedure
			if(si_station_resetprot(dev->fd, dev->prot) < 0){
				error(EXIT_SUCCESS, ERR_SETPROT, "Reset SI station setup failed for %s.", dev->devfile);
			}
			exit(0);
		default:	// Parent - dispatcher
			printf("Started client %d.\n", pid);
			newcli = (struct s_client *) malloc(sizeof(struct s_client));
			if(newcli == NULL){
				error(EXIT_FAILURE, errno, "Cannot malloc.");
			}
			newcli->pid = pid;
			newcli->next = NULL;
			if(cli == NULL){
				first_cli = newcli;
			}else{
				cli->next = newcli;
			}
			cli = newcli;
			dev = dev->next;
		}
	}
	printf("Started all clients. Press 'q' + Enter to exit.\n");
	while((c = getchar()) != 'q'){
	}
	cli = first_cli;
	while(cli != NULL){
		kill(cli->pid, SIGTERM);
		cli = cli->next;
	}
}
    
