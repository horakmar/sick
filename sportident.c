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

#define ERR_OK	          0
#define ERR_START       100
#define ERR_UNKNOWN     100
#define ERR_OPEN        101
#define ERR_GETATTR     102
#define ERR_SETATTR     103
#define ERR_NOSTART     104
#define ERR_MALLOC      105
#define ERR_BADCRC      106
#define ERR_WRITE		107
#define ERR_READ		108
#define ERR_TIMEOUT		109
#define ERR_NAK			110

int si_errno;
int si_verbose = 1;         // Verbose level of library

/****************************************************************************
 * Manipulate with error status
 ****************************************************************************/
char *si_strerror(int si_errno){
	char *errors[] = {
		"Unknown error",									// 100
		"Cannot open serial device.",						// 101
		"Cannot get serial device attributes.",				// 102
		"Cannot set serial device attributes.",				// 103
		"No start data found.",								// 104
		"No memory left for operation.",					// 105
		"Bad CRC."											// 106
	};
	int errptr = si_errno - ERR_START;
	if(errptr >= sizeof(errors) / sizeof(errors[0])) errptr = 0;
	return(errors[errptr]);
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

void si_print_hex(byte *data, uint len){
    uint i;
    char data_str[len+1];

    strncpy(data_str, (char *) data, len);
    data_str[len] = '\0';
    printf("%d: ", len);
    for(i = 0; i < len; i++){
        printf("%02X ", data[i]);
    }
    putchar('\n');
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
static int walk(char result[][PATH_MAX], int j){
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

int si_detect_devices(char result[][PATH_MAX]){
	int i, j = 0;
    char filename[PATH_MAX];

	if(chdir(SI_SEARCHDIR) != 0){
		error(0, errno, "Cannot open directory %s", SI_SEARCHDIR);
		return 0;
	}
	j = walk(result, j);
    for(i = 0; i < j; i++){
        strcpy(filename, result[i]);
        strcpy(result[i], SI_DEV_PREFIX);
        strcat(result[i], filename);
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
    termset.c_cc[VTIME] = 1;            // Wait for 0.1 delay between bytes or
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
byte *si_frame(byte *data, uint *p_len){
    uint i, j, lendata, crc;
    byte *framed_data;
    uint len = *p_len;

    if(data[0] < 0x20){                             // Control codes
        if((framed_data = (byte *) malloc(2)) == NULL){
            si_errno = 105;                         // No memory left
            return NULL;
        }
        framed_data[0] = WAKE;
        framed_data[1] = data[0];
        j = 2;
    }else if(data[0] < 0x80 || data[0] == 0xC4){    // Old protocol
        if((framed_data = (byte *) malloc(3 + 2 * len)) == NULL){
            si_errno = 105;                         // No memory left
            return NULL;
        }
        framed_data[0] = WAKE;
        framed_data[1] = STX;
        j = 2;
        for(i = 0; i < len; i++){
            if(data[i] < 0x20){
                framed_data[j++] = DLE;
            }
            framed_data[j++] = data[i];
        }
        framed_data[j++] = ETX;
    }else{                                          // New protocol
        if((framed_data = (byte *) malloc(5 + len)) == NULL){
            si_errno = 105;                         // No memory left
            return NULL;
        }
        lendata = data[1];
        crc = si_crc(lendata+2, data);
        framed_data[0] = WAKE;
        framed_data[1] = STX;
        j = 2;
        memcpy(framed_data + j, data, len);
        j += len;
        framed_data[j++] = (byte) (crc >> 8);
        framed_data[j++] = (byte) (crc & 0xFF);
        framed_data[j++] = ETX;
    }
    *p_len = j;                                     // return actual length
    return framed_data;
}

/*
 * Unframe received data
 * First two bytes of returned array is length of data (MSB LSB)
 */
byte *si_unframe(byte *data, uint len){
    uint i, j, lendata, f_dle;
    uint uf_len = 0;
    uint data_crc, comp_crc;
    byte *uf_data;

    i = 0;
    while(data[i] != STX && i < len){
        if(data[i] == NAK || data[i] == ACK){
            if((uf_data = (byte *) malloc(2 + 1)) == NULL){
                si_errno = 105;                         // No memory left
                return NULL;
            }
            uf_data[0] = 0; uf_data[1] = 1; uf_data[2] = data[i];
            return uf_data;
        }
        i++;
    }
    i++;    // i points behind STX - onto Command
    if(i >= len){
        si_errno = 104;                       // On end of data
        return NULL;
    }
    if(data[i] < 0x80 || data[i] == 0xC4){    // Old protocol - strip off DLE
        if((uf_data = malloc(2 + len - i)) == NULL){
            si_errno = 105;                         // No memory left
            return NULL;
        }
        f_dle = 0;
        j = 0;
        while(i < len){
            if(f_dle > 0){
                f_dle = 0;
            }else if(data[i] == ETX){
                break;
            }else if(data[i] == DLE){
                f_dle++;
            }else{
                uf_data[2+j++] = data[i];
            }
            i++;
        }
        uf_len = j;
    }else{                                  // New protocol
        lendata = data[i+1];
        if((uf_data = malloc(2+lendata+2)) == NULL){
            si_errno = 105;                         // No memory left
            return NULL;
        }
        data_crc = (data[i+lendata+2] << 8) + data[i+lendata+3];
        comp_crc = si_crc(lendata+2, data+i);
        if(comp_crc != data_crc){
            si_errno = 106;                         // Bad CRC
            free(uf_data);
            return NULL;
        }else{
            memcpy(uf_data+2, data+i, lendata+2);
            uf_len = lendata+2;
        }
    }
    uf_data[0] = uf_len << 8; uf_data[1] = uf_len & 0xFF;		// Length of data
    return uf_data;
}

/****************************************************************************
 * Lowlevel SI communication
 ****************************************************************************/
int si_read(int sfd, byte *buff){
    ssize_t size;

    size = read(sfd, buff, DATA_CHUNK);
    if(size == -1){
        si_errno = 104;         // Read from serial device error
        return(-1);
    }
    return(1);
}

int si_write(int sfd, byte *buff){
    ssize_t size;

    size = write(sfd, buff, DATA_CHUNK);
    if(size == -1){
        si_errno = 104;         // Read of serial device error
        return(-1);
    }
    return(1);
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

/* 
 * Write to SI station and read data back.
 * Wait timeout of miliseconds for input.
 * Try <tries> times when receiving NAK.
 * First two bytes of returned array is length of data (MSB LSB).
 */  
byte *si_handshake(int sfd, int timeout, int tries, ...){
    va_list ap;
    byte b, data_write[DATA_CHUNK], data_read[DATA_CHUNK];
    byte *data_framed = NULL, *data_unframed = NULL;
    uint len, bytes_written, bytes_read, i = 0;
    uint errcount = 0;

    va_start(ap, tries);
    while((b = (byte) va_arg(ap, int)) != EOP){
        data_write[i++] = b;
        if(i > DATA_CHUNK) break;
    }
    va_end(ap);
    
    len = i;
    data_framed = si_frame(data_write, &len);
    
    while(errcount < tries){
        if(si_verbose > 2){
            fputs(">o>> ", stdout);
            si_print_hex(data_framed, len);
        }
        bytes_written = write(sfd, data_framed, len);
        if(bytes_written != len){
			si_errno = ERR_WRITE;
            errcount++;
            continue;
        }
		free(data_unframed); data_unframed = NULL;
		if(si_read_timeout(sfd, timeout) <= 0){
			si_errno = ERR_TIMEOUT;
			errcount++;
			continue;
		}
        bytes_read = read(sfd, data_read, DATA_CHUNK);
		if(bytes_read < 0){		// some error
			si_errno = ERR_READ;
			errcount++;
            continue;
        }
		if(bytes_read > 0){		// no input
			if(si_verbose > 2){
				fputs("<i<< ", stdout);
				si_print_hex(data_read, bytes_read);
			}
			data_unframed = si_unframe(data_read, bytes_read);
			if(data_unframed[2] == NAK){
				si_errno = ERR_NAK;
				errcount++;
				continue;
			}
			si_errno = ERR_OK;
			break;
		}
    }
	free(data_framed);
	return data_unframed;
}

int si_station_detect(int sfd){
	E_SPEED speed = HIGH;
	E_INSTSET instset = NEW;
	byte c_setms;
	byte *data = NULL;

	do{
		c_setms = (instset == NEW) ? C_SETMS : O_SETMS;
		free(data);
		data = si_handshake(sfd, RTIMEOUT_MS, MAX_TRIES, c_setms, 0x01, P_MSL, EOP);		// Set local communication
		if(data == NULL){
			if(si_errno == ERR_TIMEOUT && speed == HIGH){
				speed = LOW;
				si_setspeed(sfd, speed);
				continue;
			}else{
				si_errno = ERR_UNKNOWN;
				free(data);
				return -1;
			}
		}
		if(si_errno == ERR_NAK){
			if(instset == NEW){
				instset = OLD;
				continue;
			}else{
				si_errno = ERR_UNKNOWN;
				free(data);
				return -1;
			}
		}
		free(data);
		return(speed + instset);
	}while(1);
}

