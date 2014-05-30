
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
	char devices[SI_DEVICES_MAX][PATH_MAX+1];
	int devices_num = 0;
    int i, sfd;
	char prot_old;

	si_verbose = 3;		// set si library verbose level

	if((devices_num = si_detect_devices(devices)) == 0){
		error(EXIT_FAILURE, 0, "No devices detected.");
	}
	for(i = 0; i < devices_num; i++){
		if((sfd = si_initserial(devices[i])) != -1){
			break;
    	}else{
			error(EXIT_SUCCESS, errno, "Cannot open device %s.", devices[i]);
		}
	}
	if(i >= devices_num){
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

