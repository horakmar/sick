
#include <unistd.h>
#include <stdio.h>
#include "sportident.h"

int main(int argc, char* argv[]){
	struct s_devices devices;
	int num, j;

	num = si_detect_devices(&devices, SI_DEVICES_MAX);
	for(j = 0; j < num; j++){
		printf("%d: %s\n", j, devices.devfiles[j]);
	}
    return 0;
}
