/*****************************************************************************
 * Sportident C library constants
 *
 * Author:      Martin Horak
 * Version:     1.0
 * Date:        2.6.2012
 *
 * Changes:
 *****************************************************************************
 */

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
#define O5_SHIFT 0x04		// shift to card data in response
#define O5_CN0	0x05
#define O5_CN1	0x04
#define O5_CN2	0x06
#define O5_ST	0x13
#define O5_FT	0x15
#define O5_CT	0x19
#define O5_PP	0x17
#define O5_PUNCH 0x20

#define O6_SHIFT 0x05		// shift to card data in response
#define O6_CN	0x0A
#define O6_CT	0x1C
#define O6_ET	0x20        // clear (erase) time
#define O6_ST	0x18
#define O6_FT	0x14
#define O6_PP	0x12
#define O6_PUNCH 0x300
#define O6_LN   0x30        // last name
#define O6_FN   0x44        // first name

#define O8_SHIFT 0x05		// shift to card data in response
#define O8_CN	0x18
#define O8_CT	0x08
#define O8_ST	0x0C
#define O8_FT	0x10
#define O8_PP	0x16

#define O8_PUNCH 0x88
#define O9_PUNCH 0x38
#define Op_PUNCH 0xB0
#define Ot_PUNCH 0x38
#define O10_PUNCH 0x200
#define O8_OWN   0x20       // owner data (first_name;last_name;...)

// Identification
#define ID_SI6   0x00
#define ID_SI8   0x02
#define ID_SI9   0x01
#define ID_SIp   0x04
#define ID_SIt   0x06
#define ID_SIf   0x0E
#define ID_SI10  0x0F

// Flags
#define F_NOTIME 0xEE		// No time data in SI5 (16:59:26)

// Bits in protocol
#define B_EXTENDED    0x01
#define B_AUTOSEND    0x02
#define B_HANDSHAKE   0x04


// Ending character for function handshake
#define EOP 0xFF

/* END of constants */
