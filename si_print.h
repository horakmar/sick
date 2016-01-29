/*****************************************************************************
 * Sportident C library function header 
 *
 * Author:      Martin Horak
 * Version:     1.0
 * Date:        7.4.2012
 *
 * Changes:
 ****************************************************************************/

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
void si_print_card(struct s_sidata *card, FILE *stream);
char *si_makepost_card(struct s_sidata *card);
void si_freepost(char *outdata);
