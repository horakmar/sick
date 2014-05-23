
#include <error.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

#include "sportident.h"

#define MAX_TRIES 5

// int si_init_station(

int main (void){
    char device[] = "/dev/ttyUSB1";
    int si_dev, len;
    struct timeval timeout;
    fd_set set_read, set_active;
    int nready;
    byte data[DATA_CHUNK], *data1;
	ssize_t nread;
	int a, si_desc;


	si_verbose = 3;		// set si library verbose level

    if((si_dev = si_initserial(device)) == -1){
        error(EXIT_FAILURE, 0, "Cannot open device.");
    }

	si_desc = si_station_detect(si_dev);
	printf("Detected SI: %d\n", si_desc);
//    data1 = si_handshake(si_dev, MAX_TRIES, C_SETMS, 0x01, P_MSL, EOP);

	return 0;

	len = (int) data1[0] << 8 | data1[1];
	fputs("Returned: ", stdout);
	si_print_hex(data1+2, len);
	free(data1);



    FD_ZERO (&set_active);
    FD_SET (si_dev, &set_active);

    while(1){
        set_read = set_active;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        nready = select(FD_SETSIZE, &set_read, NULL, NULL, &timeout);
        if(nready == -1){
            if(errno == EINTR){
                continue;
            }else{
                error(EXIT_FAILURE, 0, "Select failed.");
            }
        }else if(nready > 0){
            nread = read(si_dev, data, DATA_CHUNK);
			if(nread > 0){
				data1 = si_unframe(data, nread);
				len = (int) data1[0] << 8 | data1[1];
				fputs("Read: ", stdout);
				si_print_hex(data1+2, len);
				free(data1);
			}
        }else{
            puts("Tick.");
        }
    }
    return 0;
}

