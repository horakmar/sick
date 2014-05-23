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
    byte *data, *str, *ufdata;
	
	for(i = 1; i < argc; i++){
		len = strlen(argv[i]);	
        si_print_hex((byte *) argv[i], len);
        data = si_frame((byte *) argv[i], &len);
        si_print_hex(data, len);
        free(data);
	}
    str = (byte *) "\xAB\x08" "Nazdarek";
    len = strlen((char *) str);
    data = si_frame(str, &len);
    si_print_hex(data, len);
    if((ufdata = si_unframe(data, &len)) == NULL){
        si_perror("Chyba unframingu");
    }else{
        puts("Unframed:");
        si_print_hex(ufdata, len);
        free(ufdata);
    }
    free(data);
	return 0;
} 

