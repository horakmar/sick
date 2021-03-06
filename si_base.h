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
#define ERR_OLDSTATION  15
#define ERR_SETPROT		16
#define ERR_ARGS		17


/****************************************************************************
 * Server status
 ****************************************************************************/
#define STAT_OK	        0
#define STAT_NORACE     1
#define STAT_NOAUTH     2
#define STAT_NOCONN     3
#define STAT_ERROR      4
#define STAT_UNKNOWN    5

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
typedef unsigned char byte;
typedef unsigned short int uint16;
typedef unsigned int uint;
typedef unsigned int uint32;

typedef enum e_speed {
    LOW = 0,
    HIGH = 1
} E_SPEED;
typedef enum e_instset {
    OLD = 2,
    NEW = 4
} E_INSTSET;
typedef enum e_tstat {
    NONE = 0,
    H12 = 1,
    H24 = 2
} E_TSTAT;

struct s_punch {
    uint    cn;          // control number
    uint32  time;        // time in seconds
    byte    dow;         // day of week
    byte    hour;
    byte    min;
    byte    sec;
    uint16  msec;
    E_TSTAT timestat;
};

struct s_dev {
	char	*devfile;   // Name of device file (/dev/ttyUSB0)
    int     fd;         // Device file descriptor after opening
    char    prot;       // Station current protocol (for saving)
	struct  s_dev *next;
};

#define SI_NAME_MAX 20          // Max length of name on SI card
#define PUNCHES_MAX 128
struct s_sidata {
    uint32  cardnum;
    int cardtype;
    struct s_punch start;
    struct s_punch finish;
    struct s_punch clear;
    struct s_punch check;
    byte    npunch;     // Number of punches
    struct s_punch punches[PUNCHES_MAX];
    char lname[SI_NAME_MAX+1];
    char fname[SI_NAME_MAX+1];
};

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
// si_base.c
char *si_strerror(int si_errno);
void si_perror(char *prefix);

void si_clear_punch(struct s_punch *punch);
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

void si_print_hex(byte *data, uint len);


// si_readloop.c
int si_reader_s(int dev_fd, int write_fd);
int si_reader_m(struct s_dev *first_dev, int write_fd, uint tick_timeout);

// si_reader.c
int si_read_si5(int sfd, struct s_sidata *sidata);
int si_read_si6(int sfd, struct s_sidata *sidata);
int si_read_si8(int sfd, struct s_sidata *sidata);

// si_decoder.c
uint32 si_cardnum(byte si3, byte si2, byte si1, byte si0);
void si_time3(struct s_punch *punch, byte *t, char detect_null);
void si_time4(struct s_punch *punch, byte *t, char detect_null);
byte *si_name(char *name, byte *data);
char *si_timestr(char *time, struct s_punch *punch);

// si_json.c
const char *si_stat_errdesc(int err);
char *si_data_json(struct s_sidata *card);
struct s_sidata *si_json_data(struct s_sidata *card, char *jsonstr);
char *si_init_json(void);
int si_getstatus_json(char *jsonstr);
char *si_getstring_json(char *jsonstr, char *retkey);
