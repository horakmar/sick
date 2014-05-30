/*
 * Test CRC computation
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sportident.h"

void print_hex(byte *data, uint len){
    uint i;
    for(i = 0; i < len; i++){
        printf("%02X ", data[i]);
    }
    putchar('\n');
}

int main(int argc, char *argv[]) {
    uint i;
    uint len;
    byte data[DATA_CHUNK], ufdata[DATA_CHUNK], *str;
	
	for(i = 1; i < argc; i++){
		len = strlen(argv[i]);	
        si_print_hex((byte *) argv[i], len, stdout);
        len = si_frame(data, (byte *) argv[i], len);
        si_print_hex(data, len, stdout);
	}
    str = (byte *) "\xAB\x08" "Nazdarek";
    len = strlen((char *) str);
    len = si_frame(data, str, len);
    si_print_hex(data, len, stdout);
    if((len = si_unframe(ufdata, data, len)) == 0){
        si_perror("Chyba unframingu");
    }else{
        puts("Unframed:");
        si_print_hex(ufdata, len, stdout);
    }
	return 0;
} 

