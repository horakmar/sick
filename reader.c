
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <signal.h>

#include "sportident.h"

void termination_handler(int signum){
	f_term = 1;
}

#define MAX_PARAMS 256
int main(int argc, char* argv[]) {
    int i, j, k;
    int p = 0;
    char* params[MAX_PARAMS];
	struct stat st;

    int verbose = 1;
    int list = 0;
    int detect = 0;
	struct s_devices devices;
	devices.count = 0;
	
    for(i = 1; i < argc; i += k){
        k = 1;  /* counter of string parameters */
        if (argv[i][0] == '-') {
            for(j=0; argv[i][j] != '\0'; j++){
                switch(argv[i][j]){
                    case 'v':
                        verbose++;
                        break;
                    case 'q':
                      verbose--;
                        break;
                    case 'a':
                        detect = 1;
                        break;
                    case 'l':
                        detect = 1;
                        list = 1;
                        break;
					case 'd':
						if(i + k < argc){
							if(devices.count < SI_DEVICES_MAX){
								if(stat(argv[i+k], &st) == 0 && S_ISCHR(st.st_mode)){
									devices.devfiles[devices.count++] = argv[i+k];
								}else if(verbose > 1){
									printf("Device %s cannot open.\n", argv[i+k]);
								}
							}
							k++;
						}
						break;
                }
            }
        }else{
            if(p >= MAX_PARAMS) continue;
            params[p++] = argv[i];
        }
	}
	if(detect){
		si_detect_devices(&devices, SI_DEVICES_MAX);
	}
	if(1 || list){
		if(verbose > 0)	printf("Detected SI devices:\n");
		for(i = 0; i < devices.count; i++){
			printf("%d: %s\n", i, devices.devfiles[i]);
		}
		exit(EXIT_SUCCESS);
	}
}

/*
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
*/
