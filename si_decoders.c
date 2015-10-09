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

/****************************************************************************
 * Decoders
 ****************************************************************************/
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
 * Decode 2 byte time
 */
void si_time3(S_PUNCH *punch, byte *t, char detect_null){

	punch->time = (*t << 8) + *(t+1);
	if(detect_null == NULL_OK && punch->time == 0xEEEE){
		punch->time = 0;
		punch->timestat = NONE;
	}else{
		punch->timestat = H12;
	}
	punch->hour = (int) (punch->time / 3600);
	punch->min  = ((int) (punch->time / 60)) % 60;
	punch->sec  = punch->time % 60;
	return;
}

/*
 * Decode 4 byte time
 */
void si_time4(S_PUNCH *punch, byte *t, char detect_null){

	punch->time = (*t & 0x01) * 43200 + (*(t+2) << 8) + *(t+3);
	if(detect_null == NULL_OK && punch->time == 0xEEEE){
		punch->time = 0;
		punch->timestat = NONE;
	}else{
		punch->timestat = H24;
	}
	punch->hour = (int) (punch->time / 3600);
	punch->min  = ((int) (punch->time / 60)) % 60;
	punch->sec  = punch->time % 60;
	return;
}

/*
 * Reading name
 * End with double space, 20 chars or semicolon
 */
byte *si_name(char *name, byte *data){
    int i;

    if(*data >= 'A' || *data <= 'z'){
        for(i = 0; i < SI_NAME_MAX; i++){
            if(data[i] == ';' || (data[i] == ' ' && (i == SI_NAME_MAX-1 || data[i+1] == ' '))){
                i++;     // Dirty hack to shift on next name
                break;
            }
            *name = data[i];
            name++;
        }
    }
    *name = '\0';
    return data+i;
}
