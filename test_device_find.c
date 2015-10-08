
#include <unistd.h>
#include <stdio.h>
#include "sportident.h"

int main(int argc, char* argv[]){
	struct s_dev *dev_first, *devs;
	uint num;
    
	si_verbose = 3;
	num = si_detect_devices(&dev_first);
    devs = dev_first;
    printf("Detected %d device(s)\n", num);
	while(devs != NULL){
		printf("%s\n", devs->devfile);
		devs = devs->next;
	}
    return 0;
}
