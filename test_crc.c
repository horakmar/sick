/*
 * Test CRC computation
 */

#include <stdio.h>
#include <string.h>
#include "sportident.h"

int main(int argc, char *argv[]) {
int i;
unsigned short int crcsum;
unsigned int length;
unsigned char *str;
	
	for(i = 1; i < argc; i++){
		length = strlen(argv[i]);	
		crcsum = si_crc(length, (unsigned char *) argv[i]);
		printf("%d, %s: %X\n", length, argv[i], crcsum);
	}
    str = (byte *) "\xAB\x06" "Nazdar";
    length = strlen((char *) str);
    crcsum = si_crc(length, (unsigned char *) str);
    printf("%d, %s: %X\n", length, str, crcsum);
	return 0;
} 

