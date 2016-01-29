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
#include <stdlib.h>

#include "si_base.h"
#include "si_print.h"

/****************************************************************************
 * Functions
 ****************************************************************************/

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

char *si_makepost_card(struct s_sidata *card){
	char *outdata;
	int size = 1;
	char time[16];

	/* count buffer size */
	/* si_number, si_type */
	size += 17 + 10;
	
	/* si_lname */
	if(card->lname){
		size += 9 + strlen(card->lname);
	}

	/* si_fname */
	if(card->fname){
		size += 9 + strlen(card->fname);
	}

	/* tm_clear, tm_check, tm_start, tm_finish */
	size += 9 + 9 + 9 + 10 + 4*8;

	if((outdata = (char *) malloc(size)) == NULL) return NULL;
	
	sprintf(outdata, "si_number=%d&si_type=%d", card->cardnum, card->cardtype);
	if(card->lname){
	   strcat(outdata, "&si_lname=");
	   strcat(outdata, card->lname);
	}
	if(card->fname){
	   strcat(outdata, "&si_fname=");
	   strcat(outdata, card->fname);
	}
	strcat(outdata, "&tm_clear=");
	strcat(outdata, si_timestr(time, &card->clear));
	strcat(outdata, "&tm_check=");
	strcat(outdata, si_timestr(time, &card->check));
	strcat(outdata, "&tm_start=");
	strcat(outdata, si_timestr(time, &card->start));
	strcat(outdata, "&tm_finish=");
	strcat(outdata, si_timestr(time, &card->finish));
	return outdata;
}

void si_freepost(char *outdata){
	free(outdata);
}
