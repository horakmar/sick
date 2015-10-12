/*****************************************************************************
 * Sportident C library function
 *
 *
 * Author:      Martin Horak
 * Version:     1.0
 * Date:        7.4.2012
 *
 * Changes:
 *****************************************************************************
 */

/****************************************************************************
 * Includes
 ****************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <error.h> 
#include <errno.h>

#include "si_base.h"
#include "si_print.h"

int f_term = 0;

/****************************************************************************
 * Reader loop.
 ****************************************************************************/
/*
 * Loop for reading SI cards
 * When card is inserted, reads it, process data and write them to write_fd
 * Both file descriptors must be opened
 */
int si_reader_m(struct s_dev *first_dev, int write_fd, uint tick_timeout){
	struct s_sidata sidata;
	fd_set set_read, set_active;
	struct timeval tm;
	byte data_read[DATA_CHUNK], data_unframed[DATA_CHUNK];
	int nready, sfd;
	uint len;
    struct s_dev *dev;

    FD_ZERO (&set_active);
    dev = first_dev;
    while(dev != NULL){
        FD_SET (dev->fd, &set_active);
        dev = dev->next;
    }

    while(f_term == 0){
        set_read = set_active;
        tm.tv_sec = tick_timeout;
        tm.tv_usec = 0;
        sfd = -1;               // Active FD
        nready = select(FD_SETSIZE, &set_read, NULL, NULL, &tm);
        if(nready == -1){
            if(errno == EINTR){
                continue;
            }else{
				si_errno = ERR_SELECT;
                return -1;
            }
		}
        if(nready > 0){
            dev = first_dev;
            while(dev != NULL){
                if(FD_ISSET(dev->fd, &set_read)){
                    sfd = dev->fd;
                    break;
                }
                dev = dev->next;
            }
            if(sfd == -1) continue;
            len = si_read(sfd, data_read);
			if(len > 0){
				len = si_unframe(data_unframed, data_read, len);
				if(len > 1){		// Inserted SI card ...?
					si_clear_sidata(&sidata);
					switch(data_unframed[0]){
						case IN5:
							si_read_si5(sfd, &sidata);
							break;
 						case IN6:
							si_read_si6(sfd, &sidata);
							break;
 						case IN8:
							si_read_si8(sfd, &sidata);
							break;
						case OUT:
							break;
						default:
							if(si_verbose > 1){
								error(EXIT_SUCCESS, errno, "Unexpected data:\n");
								si_print_hex(data_unframed, len, stderr);
							}

					}
					if(sidata.cardnum > 0){
						if(write(write_fd, &sidata, sizeof(sidata)) != sizeof(sidata)){
							error(EXIT_SUCCESS, errno, "Not all data written to pipe.\n");
						}
					}
				}else{
					if(si_verbose > 1){
						error(EXIT_SUCCESS, errno, "Unexpected data:\n");
						si_print_hex(data_unframed, len, stderr);
					}
				}
			}else{
				if(si_verbose > 1){
					si_perror("Eror in reading");
				}
			}
        }else{
			if(si_verbose > 3){
	            error(EXIT_SUCCESS, ERR_OK, "Tick.");
			}
        }
    }
	return 0;

}
