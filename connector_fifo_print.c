/*
 * Connector of SI data FIFO to printing to screen
 */

#include <error.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>

#include "si_base.h"
#include "si_print.h"

#define FIFO_NAME "/tmp/si_data_fifo"

void termination_handler(int signum){
	f_term = 1;
}

int main(void){
    int datafd;
    int r;
	pid_t pid;
    struct s_sidata data;
	struct s_dev *first_dev, *dev, **pp_dev;

	si_verbose = 4;		// set si library verbose level

	if(sizeof(data) > PIPE_BUF){
		error(EXIT_FAILURE, ERR_SIZE, "Size of card data too big.\n");
	}

    if(access(FIFO_NAME, F_OK) == -1){
        if(mkfifo(FIFO_NAME, 0777) != 0){
            error(EXIT_FAILURE, errno, "Cannot create fifo %s.\n", FIFO_NAME);
        }
    }

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

	pid = fork();
	switch(pid){
		case -1:
			error(EXIT_FAILURE, errno, "Cannot create new process.\n");
		case 0: // SI reader
			if((datafd = open(FIFO_NAME, O_WRONLY)) == -1){
				error(EXIT_FAILURE, errno, "Cannot open fifo.\n");
			}else{
				si_reader_m(first_dev, datafd, 5);
			}
			break;
		default:
			if((datafd = open(FIFO_NAME, O_RDONLY)) == -1){
				error(EXIT_FAILURE, errno, "Cannot open fifo.\n");
			}else{
				do{
					r = read(datafd, &data, sizeof(data));
					if(r > 0){
						printf(">>> SI card:\n");
						si_print_card(&data, stdout);
					}
				}while(r > 0);
			}
			wait(NULL);
	}

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
    
