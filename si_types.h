/*****************************************************************************
 * Sportident C library function header 
 * Types definition
 *
 * Author:      Martin Horak
 * Version:     1.0
 * Date:        7.4.2012
 *
 * Changes:
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

typedef struct s_punch {
    uint    cn;          // control number
    uint32  time;        // time in seconds
    byte    dow;         // day of week
    byte    hour;
    byte    min;
    byte    sec;
    uint16  msec;
    E_TSTAT timestat;
} S_PUNCH;

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
    S_PUNCH start;
    S_PUNCH finish;
    S_PUNCH clear;
    S_PUNCH check;
    byte    npunch;     // Number of punches
    S_PUNCH punches[PUNCHES_MAX];
    char lname[SI_NAME_MAX+1];
    char fname[SI_NAME_MAX+1];
};

