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
#define WAKE 0xFF
#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define NAK 0x15
#define DLE 0x10
#define IN5 0xE5
#define IN6 0xE6
#define IN8 0xE8
#define OUT 0xE7


// Commands
#define C_SETMS 0xF0
#define C_SETSY 0x82	// params: offset, data...
#define C_GETSY 0x83	// params: offset, length
#define C_READ5 0xB1
#define C_READ6 0xE1
#define C_READ8 0xEF

// Old protocol commands
#define U_SETMS 0x70

// Command parameters
#define P_MSL   0x4D      // local communication
#define P_MSR   0x53      // remote communication

// Offsets in station memory
#define O_MODE    0x71
#define O_CN 	  0x72
#define O_PROTO   0x74

// Offsets in card memory
#define O5_SI0	0x05
#define O5_SI1	0x04
#define O5_SI2	0x06
#define O5_ST0	0x14
#define O5_ST1	0x13
#define O5_FT0	0x16
#define O5_FT1	0x15
#define O5_CT0	0x1A
#define O5_CT1	0x19
#define O5_SHIFT 0x04		// shift to card data in response

// Flags
#define F_NOTIME 0xEE		// No time data in SI5 (16:59:26)

// Bits in protocol
#define B_EXTENDED    0x01
#define B_AUTOSEND    0x02
#define B_HANDSHAKE   0x04


// Ending character for function handshake
#define EOP 0xFF


#define DATA_CHUNK 0xFF
#define SI_DEVICES_MAX 4
#define TIMEOUT_MS 200
#define TIMEOUT_READ 5000
#define MAX_TRIES 5


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

typedef struct s_punch {
	uint   cn;			// control number
	uint32 time;		// time in seconds
	byte   dow;			// day of week
	byte   hour;
	byte   min;
	byte   sec;
	uint16 msec;
} S_PUNCH;


#define SI_NAME_MAX 20
#define PUNCHES_MAX 128

struct s_sidata {
	uint32  cardnum;
	S_PUNCH   start;
	S_PUNCH	finish;
	S_PUNCH	clear;
	S_PUNCH	check;
	byte	npunch;		// Number of punches
	byte	f_24h;		// Time is in 24h format
	S_PUNCH punches[PUNCHES_MAX];
	char lname[SI_NAME_MAX+1];
	char fname[SI_NAME_MAX+1];
};

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
#ifdef DEBUG
void si_print_hex(byte *data, uint len, FILE *stream);
#endif
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

int si_detect_devices(char result[][PATH_MAX+1]);
uint si_handshake(byte *data_out, int sfd, int timeout, int tries, ...);
int si_station_detect(int sfd);
char si_station_setprot(int sfd);
char si_station_resetprot(int sfd, byte cpc);
int si_reader(int sfd, int write_fd, uint tick_timeout);
int si_read_si5(int sfd, struct s_sidata *sidata);

uint32 si_cardnum(byte si3, byte si2, byte si1, byte si0);
void si_time3(S_PUNCH *punch, byte t1, byte t0);
