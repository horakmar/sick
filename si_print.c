/*****************************************************************************
 * Sportident C library function
 *
 *
 * Author:      Martin Horak
 * Version:     1.0
 * Date:        7.4.2012
 *
 * Changes:
 ****************************************************************************/
/****************************************************************************
 * Includes
 ****************************************************************************/
#include <string.h>
#include <stdio.h>

#include "si_types.h"
#include "si_print.h"

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

char *si_timestr(char *time, S_PUNCH *punch){

	if(punch->timestat == NONE){
		strcpy(time, "--");
	}else{
		sprintf(time, "%02d:%02d:%02d", punch->hour, punch->min, punch->sec);
	}
	return time;
}

void si_print_card(struct s_sidata *card, FILE *stream){
	int i;
	char time[16];

	fputs("===================================\n", stream);
	fprintf(stream, "SI: %d (type %d)\n", card->cardnum, card->cardtype);
	fprintf(stream, "Name: %s %s\n", card->fname, card->lname);
	fputs("===================================\n", stream);

	fprintf(stream, "Clear:  %s\n", si_timestr(time, &card->clear));
	fprintf(stream, "Check:  %s\n", si_timestr(time, &card->check));
	fprintf(stream, "Start:  %s\n", si_timestr(time, &card->start));
	fprintf(stream, "Finish: %s\n", si_timestr(time, &card->finish));
	fputs("-----------------------------------\n", stream);
	for(i = 0; i < card->npunch; i++){
		fprintf(stream, "%3d ... %s\n", card->punches[i].cn, si_timestr(time, &card->punches[i]));
	}
	fputs("===================================\n", stream);
}
