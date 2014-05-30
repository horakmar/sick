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
#include <sys/stat.h>
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

#include "sportident.h"

#define ERR_OK	        0
#define ERR_UNKNOWN     0
#define ERR_OPEN        1
#define ERR_GETATTR     2
#define ERR_SETATTR     3
#define ERR_NOSTART     4
#define ERR_MALLOC      5
#define ERR_BADCRC      6
#define ERR_WRITE		7
#define ERR_READ		8
#define ERR_TIMEOUT		9
#define ERR_NAK			10
#define ERR_SELECT		11


int si_errno;
int si_verbose = 1;         // Verbose level of library
int f_term = 0;				// Termination flag

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
		"Select error."										// 11
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
 * Debug print
 ****************************************************************************/

void si_print_hex(byte *data, uint len, FILE *stream){
    uint i;
    char data_str[len+1];

    strncpy(data_str, (char *) data, len);
    data_str[len] = '\0';
    fprintf(stream, "%d: ", len);
    for(i = 0; i < len; i++){
        fprintf(stream, "%02X ", data[i]);
    }
    putc('\n', stream);
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
}

void si_clear_sidata(struct s_sidata *sidata){

	sidata->cardnum = 0;
	si_clear_punch(&sidata->start);
	si_clear_punch(&sidata->finish);
	si_clear_punch(&sidata->clear);
	si_clear_punch(&sidata->check);
	sidata->npunch = 0;		// Number of punches
	sidata->f_24h = 0;		// Time is in 24h format
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
#define SI_DEVICES_MAX 4
#define SI_DEV_PREFIX "/dev/"
#define SI_SEARCHDIR "/sys/devices"
#define SI_DEV_PATTERN "ttyUSB"
#define SI_VENDOR_ID "10c4"
#define SI_PRODUCT_ID "800a"

/* Recursive filesystem search */
static int walk(char result[][PATH_MAX+1], int j){
    uint    SI_ID_LEN = strlen(SI_VENDOR_ID);
    uint    SI_DEV_PATTERN_LEN = strlen(SI_DEV_PATTERN);
	DIR    *dir;
	struct dirent *d;
	FILE   *fi_idv, *fi_idp; 
	char   idv[SI_ID_LEN], idp[SI_ID_LEN];

    if(j >= SI_DEVICES_MAX) return j;
	if((dir = opendir(".")) == NULL){
		error(0, errno, "Cannot opendir.");
		return j;
	}
	while((d = readdir(dir))){
		if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0){
			continue;
		}
		if(d->d_type == DT_DIR){
			if(chdir(d->d_name) == 0){
    			j = walk(result, j);
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
						strcpy(result[j++], d->d_name);
					}
					fclose(fi_idp);
				}
				fclose(fi_idv);
			}
		}
	}
	closedir(dir);
	return j;
}

int si_detect_devices(char result[][PATH_MAX+1]){
	int i, j = 0;
    char filename[PATH_MAX+1];
	char cur_dir[PATH_MAX+1];

	getcwd(cur_dir, PATH_MAX);
	if(chdir(SI_SEARCHDIR) != 0){
		error(0, errno, "Cannot change to directory %s", SI_SEARCHDIR);
		return 0;
	}
	j = walk(result, j);
	if(si_verbose > 2){
		puts("Detected devices:");
	}
    for(i = 0; i < j; i++){
        strcpy(filename, result[i]);
        strcpy(result[i], SI_DEV_PREFIX);
        strcat(result[i], filename);
		if(si_verbose > 2){
			printf("%d: %s\n", i, result[i]);
		}
    }
	return j;
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
        si_errno = 101;        // Cannot open serial device
        return(-1);
    }
    if(tcgetattr(sfd, &init_termset) == -1){
        si_errno = 102;        // Cannot get serial device attributes
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
        si_errno = 103;        // Cannot set serial device attributes
        return(-1);
    }

    return(sfd);
}

/* Set read timeout of opened serial device */
int si_settimeout(int sfd, int timeout){
    struct termios termset;
    if(tcgetattr(sfd, &termset)){
        si_errno = 102;        // Cannot get serial device attributes
        return(-1);
    }
    termset.c_cc[VTIME] = timeout;
    if(tcsetattr(sfd, TCSANOW, &termset)){
        si_errno = 103;        // Cannot set serial device attributes
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
        si_errno = 102;        // Cannot get serial device attributes
        return(-1);
    }
    cfsetispeed(&termset, termspeed);
    cfsetospeed(&termset, termspeed);
    if(tcsetattr(sfd, TCSANOW, &termset)){
        si_errno = 103;        // Cannot set serial device attributes
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

/*
 * Loop for reading SI cards
 * When card is inserted, reads it, process data and write them to write_fd
 * Both file descriptors must be opened
 */
int si_reader(int sfd, int write_fd, uint tick_timeout){
	struct s_sidata sidata;
	fd_set set_read, set_active;
	struct timeval tm;
	byte data_read[DATA_CHUNK], data_unframed[DATA_CHUNK];
	int nready;
	uint len;

    FD_ZERO (&set_active);
    FD_SET (sfd, &set_active);

    while(f_term == 0){
        set_read = set_active;
        tm.tv_sec = tick_timeout;
        tm.tv_usec = 0;
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
            len = si_read(sfd, data_read);
			if(len > 0){
				len = si_unframe(data_unframed, data_read, len);
				if(len > 1){		// Inserted SI card ...?
					switch(data_unframed[0]){
						case IN5:
							si_read_si5(sfd, &sidata);
							break;
/*
 						case IN6:
							si_read_si6(sfd, write_fd);
							break;
 						case IN8:
							si_read_si8(sfd, write_fd);
							break;
*/
						case OUT:
							break;
						default:
							if(si_verbose > 1){
								fputs("Unexpected data: ", stderr);
								si_print_hex(data_unframed, len, stderr);
							}
					}
				}else{
					if(si_verbose > 1){
						fputs("Unexpected data: ", stderr);
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
	            fputs("Tick.", stderr);
			}
        }
    }
	return 0;

}

/*
 * Decode SI card number
 */
uint32 si_cardnum(byte si3, byte si2, byte si1, byte si0){
	uint32 cardnum;

	if(si2 == 1) si2 = 0;
	if(si2 <= 4){				// SI 5 card
		cardnum = 100000 * si2 + (si1 << 8) + si0;
	}else{
		cardnum = (si2 << 16) | (si1 << 8) | si0;
	}
	return cardnum;
}

/*
 * Decode 3 byte time
 */
void si_time3(S_PUNCH *punch, byte t1, byte t0){

	punch->time = (t1 << 8) | t0;
	if(punch->time == 0xEEEE) punch->time = 0;
	punch->hour = (int) (punch->time / 3600);
	punch->min  = ((int) (punch->time / 60)) % 60;
	punch->sec  = punch->time % 60;
	return;
}

/*
 * Reading SI5 cards
 */
int si_read_si5(int sfd, struct s_sidata *sidata){
	int len, len_ack;
	byte data[DATA_CHUNK], data_ack_framed[8], data_ack[] = { ACK };
	byte *p_data;
	
	si_clear_sidata(sidata);
	len = si_handshake(data, sfd, TIMEOUT_READ, MAX_TRIES, C_READ5, 0x00);
	if(len > 0){
		if(data[0] == C_READ5){
			p_data = (byte *) data + O5_SHIFT;		// Offset of card data
			sidata->cardnum = si_cardnum(0, p_data[O5_SI2], p_data[O5_SI1], p_data[O5_SI0]);
			si_time3(&sidata->start, p_data[O5_ST1], p_data[O5_ST0]);
			si_time3(&sidata->finish, p_data[O5_FT1], p_data[O5_FT0]);
			si_time3(&sidata->check, p_data[O5_CT1], p_data[O5_CT0]);
			printf(">>> cardnum = %d\n", sidata->cardnum);
			printf(">>> start time = %d ... %d:%d:%d\n", sidata->start.time, sidata->start.hour, sidata->start.min, sidata->start.sec);
			printf(">>> finish time = %d ... %d:%d:%d\n", sidata->finish.time, sidata->finish.hour, sidata->finish.min, sidata->finish.sec);
			printf(">>> check time = %d ... %d:%d:%d\n", sidata->check.time, sidata->check.hour, sidata->check.min, sidata->check.sec);
		}
	}
	
	/* Send ACK to commit finished reading */
	len_ack = si_frame(data_ack_framed, data_ack, 1);
	si_write(sfd, data_ack_framed, len_ack);

	return len;
}
