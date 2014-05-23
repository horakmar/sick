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

// Old protocol commands
#define O_SETMS 0x70

// Command parameters
#define P_MSL 0x4D      // local communication
#define P_MSR 0x53      // remote communication

// Ending character for function handshake
#define EOP 0xFF


#define DATA_CHUNK 0xFF
#define SI_DEVICES_MAX 4
#define RTIMEOUT_MS 200
#define MAX_TRIES 5

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
byte *si_frame(byte *data, uint *p_len);
byte *si_unframe(byte *data, uint len);
int si_detect_devices(char result[][PATH_MAX]);
int si_read_timeout(int sfd, int timeout);
byte *si_handshake(int sfd, int timeout, int tries, ...);
int si_station_detect(int sfd);
