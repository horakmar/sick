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

#include "si_base.h"

/****************************************************************************
 * Card readers
 ****************************************************************************/
/*
 * Reading SI5 cards
 */
int si_read_si5(int sfd, struct s_sidata *sidata){
	int len, len_ack;
	byte data[DATA_CHUNK], data_ack_framed[8], data_ack[] = { ACK };
	byte *p_data;
	int offset, i;
	
	len = si_handshake(data, sfd, TIMEOUT_READ, MAX_TRIES, C_READ5, 0x00);
	if(len > 0){
		if(data[0] == C_READ5){
			p_data = (byte *) data + O5_SHIFT;		// Offset of card data
            sidata->cardtype = 5;
			sidata->cardnum = si_cardnum(0, p_data[O5_CN2], p_data[O5_CN1], p_data[O5_CN0]);
			si_time3(&sidata->start, p_data+O5_ST, NULL_OK);
			si_time3(&sidata->finish, p_data+O5_FT, NULL_OK);
			si_time3(&sidata->check, p_data+O5_CT, NULL_OK);
			sidata->npunch = p_data[O5_PP] - 1;
			for(i = 0; i < sidata->npunch; i++){
				if(i >= 30){									// Punches 31 - 36 are without time
					offset = O5_PUNCH + (i - 30) * 0x10;
					sidata->punches[i].cn = p_data[offset];
					sidata->punches[i].timestat = NONE;
				}else{
					offset = O5_PUNCH+1 + 3*i + (int) i/5;
					sidata->punches[i].cn = p_data[offset];
					si_time3(&sidata->punches[i], p_data+offset+1, NULL_NO);
				}
			}
		}else{
			si_errno = ERR_UNDATA;
			return -1;
		}
	}else{
        // si_errno set in si_handshake
		return -1;
	}
	
	/* Send ACK to commit finished reading */
	len_ack = si_frame(data_ack_framed, data_ack, 1);
	si_write(sfd, data_ack_framed, len_ack);

	return len;
}

/*
 * Reading SI6 cards
 */
int si_read_si6(int sfd, struct s_sidata *sidata){
	int len, len_ack;
	byte data[DATA_CHUNK], data_ack_framed[8], data_ack[] = { ACK };
	byte *p_data;
	byte block = 0;
	int offset, i;
	
	do{
		len = si_handshake(data, sfd, TIMEOUT_READ, MAX_TRIES, C_READ6, 0x01, block);
		if(len > 0){
			if(data[0] == C_READ6){
				if(block == 0){					            // First block
					p_data = (byte *) data + O6_SHIFT;		// Offset of card data
                    sidata->cardtype = 6;
					sidata->cardnum = si_cardnum(p_data[O6_CN], p_data[O6_CN+1], p_data[O6_CN+2], p_data[O6_CN+3]);
					si_time4(&sidata->start, p_data+O6_ST, NULL_OK);
					si_time4(&sidata->finish, p_data+O6_FT, NULL_OK);
					si_time4(&sidata->check, p_data+O6_CT, NULL_OK);
					si_time4(&sidata->clear, p_data+O6_ET, NULL_OK);
                    si_name((char *) &sidata->fname, p_data+O6_FN);
                    si_name((char *) &sidata->lname, p_data+O6_LN);
					sidata->npunch = p_data[O6_PP];
                    i = 0;
                    offset = O6_PUNCH;
				}
                while(i < sidata->npunch && offset < BLOCK_SIZE){
                    sidata->punches[i].cn = p_data[offset+1];
                    si_time4(&sidata->punches[i], p_data+offset, NULL_NO);
                    i++; offset += 4;
				}
			}else{
                si_errno = ERR_UNDATA;
                return -1;
            }
		}else{
            // si_errno set in si_handshake
            return -1;
        }
        // Skip blocks we do not need
        while(offset >= BLOCK_SIZE){
            block++;
            offset -= BLOCK_SIZE;
        }
	}while(i < sidata->npunch);

	/* Send ACK to commit finished reading */
	len_ack = si_frame(data_ack_framed, data_ack, 1);
	si_write(sfd, data_ack_framed, len_ack);

	return len;
}

/*
 * Reading SI8 - SI10 cards
 */
int si_read_si8(int sfd, struct s_sidata *sidata){
	int len, len_ack;
	byte data[DATA_CHUNK], data_ack_framed[8], data_ack[] = { ACK };
	byte *p_data, *p_lname;
	byte block = 0;
	int offset, i;
	
	do{
		len = si_handshake(data, sfd, TIMEOUT_READ, MAX_TRIES, C_READ8, 0x01, block);
		if(len > 0){
			if(data[0] == C_READ8){
				if(block == 0){					// First block
					p_data = (byte *) data + O8_SHIFT;		// Offset of card data
					sidata->cardnum = si_cardnum(p_data[O8_CN], p_data[O8_CN+1], p_data[O8_CN+2], p_data[O8_CN+3]);
					si_time4(&sidata->start, p_data+O8_ST, NULL_OK);
					si_time4(&sidata->finish, p_data+O8_FT, NULL_OK);
					si_time4(&sidata->check, p_data+O8_CT, NULL_OK);
                    p_lname = si_name((char *) &sidata->fname, p_data+O8_OWN);
                    si_name((char *) &sidata->lname, p_lname);
					sidata->npunch = p_data[O8_PP];
                    i = 0;
					switch(p_data[O8_CN]){
						case ID_SI8:
                            sidata->cardtype = 8;
							offset = O8_PUNCH;
							break;
						case ID_SI9:
                            sidata->cardtype = 9;
							offset = O9_PUNCH;
							break;
						case ID_SIp:
                            sidata->cardtype = 21;
							offset = Op_PUNCH;
							break;
						case ID_SIt:
                            sidata->cardtype = 22;
							offset = Ot_PUNCH;
							break;
						case ID_SI10:
                            sidata->cardtype = 10;
							offset = O10_PUNCH;
							break;
						default:
							si_errno = ERR_UNKNCARD;
							return -1;
					}
				}
                while(i < sidata->npunch && offset < BLOCK_SIZE){
                    sidata->punches[i].cn = p_data[offset+1];
                    si_time4(&sidata->punches[i], p_data+offset, NULL_NO);
                    i++; offset += 4;
				}
			}else{
                si_errno = ERR_UNDATA;
                return -1;
            }
		}else{
            // si_errno set in si_handshake
            return -1;
        }
        // Skip blocks we do not need
        while(offset >= BLOCK_SIZE){
            block++;
            offset -= BLOCK_SIZE;
        }
	}while(i < sidata->npunch);

	/* Send ACK to commit finished reading */
	len_ack = si_frame(data_ack_framed, data_ack, 1);
	si_write(sfd, data_ack_framed, len_ack);

	return len;
}

