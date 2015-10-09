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

#include "sportident.h"

#define FIFO_NAME "/tmp/si_data_fifo"

int main(void){
    int datafd;
    int r;
    char buffer[PIPE_BUF+1];

    if(access(FIFO_NAME, F_OK) == -1){
        if(mkfifo(FIFO_NAME, 0777) != 0){
            error(EXIT_FAILURE, errno, "Cannot create fifo %s.\n", FIFO_NAME);
        }
    }
    if((pipefd = open(FIFO_NAME, O_RDONLY)) == -1){
        error(EXIT_FAILURE, errno, "Cannot open fifo.\n");
    }else{
        do{
            buffer[0] = '\0';
            r = read(pipefd, buffer, PIPE_BUF);
            printf("%s", buffer);
        }while(r > 0);
    }
}
    
