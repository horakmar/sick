
#include <error.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>

#include "sportident.h"

void termination_handler(int signum){
	f_term = 1;
}

int main (void){
    int i, sfd;
	char prot_old;
	struct s_devices devices;
	devices.count = 0;

	si_verbose = 3;		// set si library verbose level

	if(si_detect_devices(&devices, SI_DEVICES_MAX) == 0){
		error(EXIT_FAILURE, 0, "No devices detected.");
	}
	for(i = 0; i < devices.count; i++){
		if((sfd = si_initserial(devices.devfiles[i])) != -1){
			break;
    	}else{
			error(EXIT_SUCCESS, errno, "Cannot open device %s", devices.devfiles[i]);
		}
	}
	if(i >= devices.count){
		error(EXIT_FAILURE, 0, "No devices can be opened.");
	}

	if(! IS_NEW(si_station_detect(sfd))){
		error(EXIT_FAILURE, 0, "Old SI station not supported.");
	}


	signal(SIGINT, termination_handler);
	signal(SIGQUIT, termination_handler);
	signal(SIGTERM, termination_handler);
	signal(SIGHUP, termination_handler);

	
	if((prot_old = si_station_setprot(sfd)) == -1){
		error(EXIT_FAILURE, 0, "Cannot setup SI station protocol.");
	}

	si_reader(sfd, fileno(stdout), TIMEOUT_MS);

	if(si_station_resetprot(sfd, prot_old) < 0){
		error(EXIT_FAILURE, 0, "Reset SI station setup failed.");
	}
    return 0;
}

