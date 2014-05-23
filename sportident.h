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

// Commands
#define C_SETMS 0xF0
#define C_SETSY 0x82	// params: offset, data...
#define C_GETSY 0x83	// params: offset, length

// Old protocol commands
#define U_SETMS 0x70

// Command parameters
#define P_MSL   0x4D      // local communication
#define P_MSR   0x53      // remote communication

// Offsets in station memory
#define O_MODE    0x71
#define O_CN 	  0x72
#define O_PROTO   0x74

// Bits in protocol
#define B_EXTENDED    0x01
#define B_AUTOSEND    0x02
#define B_HANDSHAKE   0x04


// Ending character for function handshake
#define EOP 0xFF


#define DATA_CHUNK 0xFF
#define SI_DEVICES_MAX 4
#define RTIMEOUT_MS 200
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

/****************************************************************************
 * Types
 ****************************************************************************/
typedef enum e_speed {
    LOW = 0,
	HIGH = 1
} E_SPEED;
typedef enum e_instset {
	OLD = 2,
	NEW = 4
} E_INSTSET;

typedef unsigned char byte;
typedef unsigned short int uint16;
typedef unsigned int uint;

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
#ifdef DEBUG
void si_print_hex(byte *data, uint len);
#endif
char *si_strerror(int si_errno);
void si_perror(char *prefix);

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
