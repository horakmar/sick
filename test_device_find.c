
#include <unistd.h>
#include <stdio.h>
#include "sportident.h"

int main(int argc, char* argv[]){
	char devices[SI_DEVICES_MAX][PATH_MAX];
	int num, j;
    char cur_dir[PATH_MAX+1];

    getcwd(cur_dir, PATH_MAX);
	num = si_detect_devices(devices);
	for(j = 0; j < num; j++){
		printf("%d: %s\n", j, devices[j]);
	}
    chdir(cur_dir);
    return 0;
}
