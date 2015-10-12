/*****************************************************************************
 * Sportident C library function header 
 *
 * Author:      Martin Horak
 * Version:     1.0
 * Date:        7.4.2012
 *
 * Changes:
 ****************************************************************************/

#include <stdio.h> 

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
void si_print_hex(byte *data, uint len, FILE *stream);
char *si_timestr(char *time, S_PUNCH *punch);
void si_print_card(struct s_sidata *card, FILE *stream);

