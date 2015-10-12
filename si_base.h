/*****************************************************************************
 * Sportident C library function header 
 *
 * Author:      Martin Horak
 * Version:     1.0
 * Date:        7.4.2012
 *
 * Changes:
 *****************************************************************************
 */

#include <sys/types.h>
#include <linux/limits.h>

/****************************************************************************
 * Constants
 ****************************************************************************/
#include "si_const.h"

#define DATA_CHUNK 0xFF
#define BLOCK_SIZE 0x80
#define TIMEOUT_MS 200
#define TIMEOUT_READ 5000
#define MAX_TRIES 5

#define NULL_NO 0
#define NULL_OK 1

/****************************************************************************
 * Errors
 ****************************************************************************/
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
#define ERR_UNDATA		12
#define ERR_UNKNCARD	13
#define ERR_SIZE		14


/****************************************************************************
 * Macros
 ****************************************************************************/
#define IS_NEW(X) ((X) >> 1)

/****************************************************************************
 * Variables
 ****************************************************************************/
extern int si_errno;
extern int si_verbose;
extern int f_term;

/****************************************************************************
 * Types
 ****************************************************************************/
#include "si_types.h"

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
// si_base.c
char *si_strerror(int si_errno);
void si_perror(char *prefix);

void si_clear_punch(S_PUNCH *punch);
void si_clear_sidata(struct s_sidata *sidata);

uint16 si_crc(uint Length, byte *p_Data);

int si_initserial(char *serial_device);
int si_setspeed(int sfd, E_SPEED speed);
int si_settimeout(int sfd, int speed);

uint si_frame(byte *data_out, byte *data_in, uint len_in);
uint si_unframe(byte *data_out, byte *data_in, uint len_in);

int si_read(int sfd, byte *buff);
int si_write(int sfd, byte *buff, uint len);

int si_read_timeout(int sfd, int timeout);

int si_detect_devices(struct s_dev **dev_first);
uint si_handshake(byte *data_out, int sfd, int timeout, int tries, ...);
int si_station_detect(int sfd);
char si_station_setprot(int sfd);
char si_station_resetprot(int sfd, byte cpc);

// si_readloop.c
int si_reader_m(struct s_dev *first_dev, int write_fd, uint tick_timeout);

// si_reader.c
int si_read_si5(int sfd, struct s_sidata *sidata);
int si_read_si6(int sfd, struct s_sidata *sidata);
int si_read_si8(int sfd, struct s_sidata *sidata);

// si_decoder.c
uint32 si_cardnum(byte si3, byte si2, byte si1, byte si0);
void si_time3(S_PUNCH *punch, byte *t, char detect_null);
void si_time4(S_PUNCH *punch, byte *t, char detect_null);
byte *si_name(char *name, byte *data);
