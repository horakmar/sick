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
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h> 
#include <stdarg.h>
#include <errno.h>

#include "si_base.h"
#include "si_print.h"

int si_errno;
int si_verbose = 1;         // Verbose level of library

/****************************************************************************
 * Manipulate with error status
 ****************************************************************************/
char *si_strerror(int si_err){
	char *errors[] = {
		"Unknown error",									// 0
		"Cannot open serial device.",						// 1
		"Cannot get serial device attributes.",				// 2
		"Cannot set serial device attributes.",				// 3
		"No start data found.",								// 4
		"No memory left for operation.",					// 5
		"Bad CRC.",											// 6
		"SI write error.",									// 7
		"SI read error.",									// 8
		"Timeout passed.",									// 9
		"NAK received.",									// 10
		"Select error.",									// 11
		"Unexpected data.",									// 12
		"Unknown SI card."									// 13
	};
	if(si_err >= sizeof(errors) / sizeof(errors[0])) si_err = 0;
	return(errors[si_err]);
}

void si_perror(char *prefix){
	fputs(prefix, stderr);
	fputs(": ", stderr);
	fputs(si_strerror(si_errno), stderr);
	putc('\n', stderr);
}

/****************************************************************************
 * Data manipulation
 ****************************************************************************/
void si_clear_punch(S_PUNCH *punch){

	punch->cn = 0;			// control number
	punch->time = 0;		// time in sec
	punch->dow = 0;			// day of week
	punch->hour = 0;
	punch->min = 0;
	punch->sec = 0;
	punch->msec = 0;
	punch->timestat = NONE;
}

void si_clear_sidata(struct s_sidata *sidata){

	sidata->cardnum = 0;
	sidata->cardtype = 0;
	si_clear_punch(&sidata->start);
	si_clear_punch(&sidata->finish);
	si_clear_punch(&sidata->clear);
	si_clear_punch(&sidata->check);
	sidata->npunch = 0;		// Number of punches
	sidata->lname[0] = '\0';
	sidata->fname[0] = '\0';
}


/****************************************************************************
 * CRC - Author: Jurgen Ehms
 ****************************************************************************/
#define POLYNOM 0x8005
#define BITMASK 0x8000

uint16 si_crc(uint Length, byte *p_Data) {
short int i;
uint16 j, Sum, Sum1;

	if (Length < 2) return(0);             // response value is "0" for none or one data byte

	Sum = *p_Data++;
	Sum = (Sum << 8) + *p_Data++;

	if (Length == 2) return(Sum);          // response value is CRC for two data bytes

	for (i = (int) (Length >> 1); i > 0; i--) {
		if (i > 1) {
			Sum1 = *p_Data++;
			Sum1 = (Sum1 << 8) + *p_Data++;
		}else{
			if (Length & 1) {              // odd number of data bytes, complete with "0"
				Sum1 = *p_Data;
				Sum1 = (Sum1 << 8);
			}else{
				Sum1 = 0;
			}
		} 
		  
		for (j=0; j<16; j++) {
			if (Sum & BITMASK) {
			    Sum <<= 1;
			    if (Sum1 & BITMASK) Sum++;
			    Sum ^= POLYNOM;
			}else{
			    Sum <<= 1;
			    if (Sum1 & BITMASK) Sum++;
			}
			Sum1 <<= 1;
		}
	}
	return(Sum);
}

/****************************************************************************
 * SI hardware detection
 ****************************************************************************/
#define SI_DEV_PREFIX "/dev/"
#define SI_SEARCHDIR "/sys/devices"
#define SI_DEV_PATTERN "ttyUSB"
#define SI_VENDOR_ID "10c4"
#define SI_PRODUCT_ID "800a"

/* Recursive filesystem search */
static int walk(struct s_dev **dev_first, struct s_dev **dev_last){
    uint    SI_ID_LEN = strlen(SI_VENDOR_ID);
    uint    SI_DEV_PATTERN_LEN = strlen(SI_DEV_PATTERN);
	DIR     *dir;
	struct  dirent *d;
	FILE    *fi_idv, *fi_idp; 
	char    idv[SI_ID_LEN], idp[SI_ID_LEN];
	struct  s_dev *p_dev;

	if((dir = opendir(".")) == NULL){
		error(0, errno, "Cannot opendir.");
		return 0;
	}
	while((d = readdir(dir))){
		if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0){
			continue;
		}
		if(d->d_type == DT_DIR){
			if(chdir(d->d_name) == 0){
                if(si_verbose > 2){
                    printf(">>> Entering directory %s\n", d->d_name);
                }
    			walk(dev_first, dev_last);
                chdir("..");
            }else{
                error(0, errno, "Cannot open directory %s", d->d_name);
            }
		}
		if(strncmp(SI_DEV_PATTERN, d->d_name, SI_DEV_PATTERN_LEN) == 0){
			if((fi_idv = fopen("../idVendor", "r")) != NULL){
				if((fi_idp = fopen("../idProduct", "r")) != NULL){
					if(fread(idv, SI_ID_LEN, 1, fi_idv) > 0 &&
					strncmp(idv, SI_VENDOR_ID, SI_ID_LEN) == 0 &&
					fread(idp, SI_ID_LEN, 1, fi_idp) > 0 &&
					strncmp(idp, SI_PRODUCT_ID, SI_ID_LEN) == 0){
						if((p_dev = (struct s_dev *) malloc(sizeof(struct s_dev))) == NULL){
							error(1, errno, "Cannot malloc.");
						}
						if((p_dev->devfile = malloc(sizeof(d->d_name) + sizeof(SI_DEV_PREFIX))) == NULL){
							error(1, errno, "Cannot malloc.");
						}
						strcpy(p_dev->devfile, SI_DEV_PREFIX);
                        strcat(p_dev->devfile, d->d_name);
                        p_dev->next = NULL;
						if(*dev_first == NULL){
                            *dev_first = p_dev;
                        }else{
							(*dev_last)->next = p_dev;
						}
						*dev_last = p_dev;
					}
					fclose(fi_idp);
				}
				fclose(fi_idv);
			}
		}
	}
	closedir(dir);
	return 1;
}

int si_detect_devices(struct s_dev **dev_first){
	struct s_dev *i = NULL;
	uint   count = 0;
	char   cur_dir[PATH_MAX+1];

	getcwd(cur_dir, PATH_MAX);
	if(chdir(SI_SEARCHDIR) != 0){
		error(0, errno, "Cannot change to directory %s.", SI_SEARCHDIR);
		return 0;
	}
    *dev_first = NULL;
    walk(dev_first, &i);
	if(si_verbose > 2){
		puts("Detected devices:");
	}
	i = *dev_first;
    while(i != NULL){
		count++;
		if(si_verbose > 2){
			printf("%d: %s\n", count, i->devfile);
		}
		i = i->next;
    }
	return count;
}

/****************************************************************************
 * Serial (TTY) functions
 ****************************************************************************/
/* Open serial device, initial settings */
int si_initserial(char *serial_device){
    int sfd;
    struct termios init_termset, termset;
    sfd = open(serial_device, O_RDWR);
    if(sfd == -1){
        si_errno = ERR_OPEN;        // Cannot open serial device
        return(-1);
    }
    if(tcgetattr(sfd, &init_termset) == -1){
        si_errno = ERR_GETATTR;        // Cannot get serial device attributes
        return(-1);
    }
    termset = init_termset;
    termset.c_lflag &= ~(ECHO | ECHOK | ICANON | ISIG);
    termset.c_iflag &= ~(BRKINT | ICRNL | IGNCR | INLCR | INPCK | ISTRIP | IXOFF | IXON | PARMRK);
    termset.c_oflag &= ~OPOST;
    termset.c_cc[VTIME] = 1;             // Wait for 0.1 delay between bytes or
    termset.c_cc[VMIN] = DATA_CHUNK;     // Return after reading all buffer
    cfsetispeed(&termset, B38400);

    if(tcsetattr(sfd, TCSANOW, &termset) == -1){
        si_errno = ERR_SETATTR;        // Cannot set serial device attributes
        return(-1);
    }

    return(sfd);
}

/* Set read timeout of opened serial device */
int si_settimeout(int sfd, int timeout){
    struct termios termset;
    if(tcgetattr(sfd, &termset)){
        si_errno = ERR_GETATTR;        // Cannot get serial device attributes
        return(-1);
    }
    termset.c_cc[VTIME] = timeout;
    if(tcsetattr(sfd, TCSANOW, &termset)){
        si_errno = ERR_SETATTR;        // Cannot set serial device attributes
        return(-1);
    }
    return(1);
}

/* Set communication speed of opened serial device */
int si_setspeed(int sfd, E_SPEED speed){
    speed_t termspeed;
    struct termios termset;
    switch(speed){
        case LOW: termspeed = B4800; break;
        case HIGH: termspeed = B38400; break;
    }
    if(tcgetattr(sfd, &termset)){
        si_errno = ERR_GETATTR;        // Cannot get serial device attributes
        return(-1);
    }
    cfsetispeed(&termset, termspeed);
    cfsetospeed(&termset, termspeed);
    if(tcsetattr(sfd, TCSANOW, &termset)){
        si_errno = ERR_SETATTR;        // Cannot set serial device attributes
        return(-1);
    }
    return(1);
}

/****************************************************************************
 * Lowlevel SI data manipulation
 ****************************************************************************/
uint si_frame(byte *data_out, byte *data_in, uint len_in){
    uint i, len_out, lendata, crc;

    if(data_in[0] < 0x20){                             // Control codes
        data_out[0] = WAKE;
        data_out[1] = data_in[0];
        len_out = 2;
    }else if(data_in[0] < 0x80 || data_in[0] == 0xC4){    // Old protocol
        data_out[0] = WAKE;
        data_out[1] = STX;
        len_out = 2;
        for(i = 0; i < len_in; i++){
            if(data_in[i] < 0x20){
                data_out[len_out++] = DLE;
            }
            data_out[len_out++] = data_in[i];
        }
        data_out[len_out++] = ETX;
    }else{                                          // New protocol
        lendata = data_in[1];
        crc = si_crc(lendata+2, data_in);
        data_out[0] = WAKE;
        data_out[1] = STX;
        len_out = 2;
        memcpy(data_out + len_out, data_in, lendata+2);
        len_out += lendata+2;
        data_out[len_out++] = (byte) (crc >> 8);
        data_out[len_out++] = (byte) (crc & 0xFF);
        data_out[len_out++] = ETX;
    }
    return len_out;
}

/*
 * Unframe received data
 * First two bytes of returned array is length of data (MSB LSB)
 */
uint si_unframe(byte *data_out, byte *data_in, uint len_in){
    uint lendata, f_dle;
    uint i = 0, len_out = 0;
    uint data_crc, comp_crc;

    while(data_in[i] != STX && i < len_in){
        if(data_in[i] == NAK || data_in[i] == ACK){
            data_out[len_out++] = data_in[i];
            return len_out;
        }
        i++;
    }
    i++;    // i points behind STX - onto Command

    if(i >= len_in){
        si_errno = ERR_NOSTART;                     // On end of data
        return len_out;
    }
    if(data_in[i] < 0x80 || data_in[i] == 0xC4){    // Old protocol - strip off DLE
        f_dle = 0;
        while(i < len_in){
            if(data_in[i] == ETX){
                break;
            }else if(data_in[i] == DLE){
                f_dle++;
            }else{
                data_out[len_out++] = data_in[i];
            }
            i++;
        }
    }else{                                  // New protocol
        lendata = data_in[i+1];
        data_crc = (data_in[i+lendata+2] << 8) + data_in[i+lendata+3];
        comp_crc = si_crc(lendata+2, data_in+i);
        if(comp_crc != data_crc){
            si_errno = ERR_BADCRC;                         // Bad CRC
            return len_out;
        }else{
            memcpy(data_out, data_in+i, lendata+2);
            len_out = lendata+2;
        }
    }
    return len_out;
}

/****************************************************************************
 * Lowlevel SI communication. Just simple wrappers for debug output
 ****************************************************************************/
int si_read(int sfd, byte *buff){
    ssize_t size;

    size = read(sfd, buff, DATA_CHUNK);
	if(size > 0){
		if(si_verbose > 2){
			fputs("<i<< ", stderr);
			si_print_hex(buff, size, stderr);
		}
	}
	return size;
}

int si_write(int sfd, byte *buff, uint len){
    ssize_t size;

	if(si_verbose > 2){
		fputs(">o>> ", stderr);
		si_print_hex(buff, len, stderr);
	}
    return(size = write(sfd, buff, len));
}

/*
 * Use select to make read timeout (in msec)
 */ 
int si_read_timeout(int sfd, int timeout){
    fd_set set_read;
    struct timeval time;
    int nready;
    int fdmax = sfd + 1;

    FD_ZERO(&set_read);
    FD_SET(sfd, &set_read);

    time.tv_sec = 0;
    time.tv_usec = timeout * 1000;

    do{
        nready = select(fdmax, &set_read, NULL, NULL, &time);
    }while(nready == -1 && errno == EINTR);
    return nready;
}

/****************************************************************************
 * Midlevel SI communication.
 ****************************************************************************/

/* 
 * Write to SI station and read data back.
 * Wait timeout of miliseconds for input.
 * Try <tries> times when receiving NAK.
 * First two bytes of returned array is length of data (MSB LSB).
 */  
uint si_handshake(byte *data_out, int sfd, int timeout, int tries, ...){
    va_list ap;
    byte b, data_write[DATA_CHUNK], data_framed[DATA_CHUNK], data_read[DATA_CHUNK];
    uint len, bytes_written, bytes_read, i = 0;
	uint len_out = 0, errcount = 0;

    va_start(ap, tries);
    while((b = (byte) va_arg(ap, int)) != EOP){
        data_write[i++] = b;
        if(i > DATA_CHUNK) break;
    }
    va_end(ap);
    
    len = i;
    len = si_frame(data_framed, data_write, len);
    
    while(errcount < tries){

        bytes_written = si_write(sfd, data_framed, len);
        if(bytes_written != len){
			si_errno = ERR_WRITE;
            errcount++;
            continue;
        }

		if(si_read_timeout(sfd, timeout) <= 0){
			si_errno = ERR_TIMEOUT;
			break;
		}

        bytes_read = si_read(sfd, data_read);
		if(bytes_read < 0){		// some error
			si_errno = ERR_READ;
			errcount++;
            continue;
        }
		if(bytes_read > 0){		// no input
			len_out = si_unframe(data_out, data_read, bytes_read);
			if(len_out <= 0){
				errcount++;
				continue;
			}
			if(data_out[0] == NAK){
				si_errno = ERR_NAK;
				len_out = 0;
				errcount++;
				continue;
			}
			si_errno = ERR_OK;
			break;
		}
    }
	return len_out;
}

int si_station_detect(int sfd){
	E_SPEED speed = HIGH;
	E_INSTSET instset = NEW;
	byte data[DATA_CHUNK], c_setms;
	uint len = 0;
	int detected = 0;

	do{
		c_setms = (instset == NEW) ? C_SETMS : U_SETMS;
		len = si_handshake(data, sfd, TIMEOUT_MS, MAX_TRIES, c_setms, 1, P_MSL, EOP);		// Set local communication
		if(len <= 0){
			if(si_errno == ERR_TIMEOUT && speed == HIGH){
				speed = LOW;
				si_setspeed(sfd, speed);
				continue;
			}else{
				si_errno = ERR_UNKNOWN;
			}
		}else if(si_errno == ERR_NAK){
			if(instset == NEW){
				instset = OLD;
				continue;
			}else{
				si_errno = ERR_UNKNOWN;
			}
		}else{
			detected = 1;
		}
		break;
	}while(1);
	if(si_verbose > 2){
		if(detected > 0){
			printf("Detected SI station: %s", (instset == NEW) ? "BSM7/8" : "BSM3/4/6");
			printf(" at speed %s.\n", (speed == HIGH) ? "HIGH" : "LOW");
		}else{
			puts("No SI station detected.");
		}
	}
	return(speed + instset);
}

/*
 * Set protocol to extended, autosend to OFF
 * Return actual protocol value
 */
char si_station_setprot(int sfd){
	byte cpc, cpc_old, data[DATA_CHUNK];
	uint len;

	len = si_handshake(data, sfd, TIMEOUT_MS, MAX_TRIES, C_GETSY, 2, O_PROTO, 1);
	if(len <= 0){
		return -1;
	}
	cpc_old = cpc = data[5];
	cpc |= B_EXTENDED | B_HANDSHAKE;
	cpc &= ~B_AUTOSEND;

	len = si_handshake(data, sfd, TIMEOUT_MS, MAX_TRIES, C_SETSY, 2, O_PROTO, cpc);
	if(len <= 0){
		return -1;
	}
	return (char) cpc_old;
}

/*
 * Reset protocol to (saved) cpc value
 */
char si_station_resetprot(int sfd, byte cpc){
	byte data[DATA_CHUNK];
	uint len;

	len = si_handshake(data, sfd, TIMEOUT_MS, MAX_TRIES, C_SETSY, 2, O_PROTO, cpc);
	if(len <= 0){
		return -1;
	}
	return cpc;
}

