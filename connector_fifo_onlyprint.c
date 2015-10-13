/*
 * Connector of SI data FIFO to printing to screen
 */

#include <error.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "si_base.h"
#include "si_print.h"

#define FIFO_NAME "/tmp/si_data_fifo"

int main(void){
    int datafd;
    int r;
    struct s_sidata data;

	si_verbose = 1;		// set si library verbose level

	if(sizeof(data) > PIPE_BUF){
		error(EXIT_FAILURE, ERR_SIZE, "Size of card data too big.\n");
	}

    if(access(FIFO_NAME, F_OK) == -1){
        if(mkfifo(FIFO_NAME, 0777) != 0){
            error(EXIT_FAILURE, errno, "Cannot create fifo %s.\n", FIFO_NAME);
        }
    }

	if((datafd = open(FIFO_NAME, O_RDONLY)) == -1){
		error(EXIT_FAILURE, errno, "Cannot open fifo.\n");
	}else{
		do{
			r = read(datafd, &data, sizeof(data));
			if(r > 0){
				si_print_card(&data, stdout);
			}
		}while(r > 0);
	}
    return 0;
}
    
