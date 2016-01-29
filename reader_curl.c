/*
 * Connector of SI data FIFO to printing to screen
 */

#include <error.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>

#include "si_base.h"
#include "si_print.h"

#include <json-c/json.h>
#include <curl/curl.h>


#define FIFO_NAME "/tmp/si_data_fifo"
#define READER_TICK_TIMER 60 			// seconds

void termination_handler(int signum){
	f_term = 1;
}

char *si_data_json(struct s_sidata *card){
	json_object *jo, *ji, *jj, *ja;
	int i;
	const char *result;
	char *jsonstr;

	jo = json_object_new_object();
	
	ji = json_object_new_int(card->cardnum);
	json_object_object_add(jo, "si_number", ji);

	ji = json_object_new_int(card->cardtype);
	json_object_object_add(jo, "si_type", ji);

	if(card->lname){
		ji = json_object_new_string(card->lname);
		json_object_object_add(jo, "si_lname", ji);
	}
	if(card->fname){
		ji = json_object_new_string(card->fname);
		json_object_object_add(jo, "si_fname", ji);
	}
	ji = json_object_new_int(card->start.time);
	json_object_object_add(jo, "tm_start", ji);
	ji = json_object_new_int(card->check.time);
	json_object_object_add(jo, "tm_check", ji);
	ji = json_object_new_int(card->clear.time);
	json_object_object_add(jo, "tm_clear", ji);
	ji = json_object_new_int(card->finish.time);
	json_object_object_add(jo, "tm_finish", ji);

	ja = json_object_new_array();
	for(i = 0; i < card->npunch; i++){
		jj = json_object_new_object();
		ji = json_object_new_int(card->punches[i].cn);
		json_object_object_add(jj, "cn", ji);
		ji = json_object_new_int(card->punches[i].time);
		json_object_object_add(jj, "time", ji);
		ji = json_object_new_int(card->punches[i].timestat);
		json_object_object_add(jj, "tst", ji);
		json_object_array_add(ja, jj);
	}
	json_object_object_add(jo, "punches", ja);

	result = json_object_to_json_string(jo);

	if((jsonstr = (char *) malloc(strlen(result))) == NULL){
		json_object_put(jo);
		return NULL;
	}
	strcpy(jsonstr, result);
	json_object_put(jo);
	return jsonstr;
}

int main(void){
    int datafd;
    int r;
	pid_t pid;
    struct s_sidata data;
	struct s_dev *first_dev, *dev, **pp_dev;
	char *postdata;
	CURL *curl;
	struct curl_slist *headers;

	si_verbose = 1;		// set si library verbose level

	if(sizeof(data) > PIPE_BUF){
		error(EXIT_FAILURE, ERR_SIZE, "Size of card data too big.\n");
	}

    if(access(FIFO_NAME, F_OK) == -1){
        if(mkfifo(FIFO_NAME, 0777) != 0){
            error(EXIT_FAILURE, errno, "Cannot create fifo %s.\n", FIFO_NAME);
        }
    }

	if(si_detect_devices(&first_dev) == 0){
		error(EXIT_FAILURE, 0, "No SI devices detected.");
	}

    pp_dev = &first_dev;
	while(*pp_dev != NULL){
		if(((*pp_dev)->fd = si_initserial((*pp_dev)->devfile)) == -1){
			error(EXIT_SUCCESS, errno, "Cannot open device %s", (*pp_dev)->devfile);
            *pp_dev = (*pp_dev)->next; // Excluded from list, but NOT freed from memory
		}else if(! IS_NEW(si_station_detect((*pp_dev)->fd))){
            error(EXIT_SUCCESS, 0, "Old SI station (%s) is not supported.", (*pp_dev)->devfile);
            *pp_dev = (*pp_dev)->next; // Excluded from list, but NOT freed from memory
        }else if(((*pp_dev)->prot = si_station_setprot((*pp_dev)->fd)) == -1){
	    	error(EXIT_SUCCESS, 0, "Cannot setup SI station (%s) protocol.", (*pp_dev)->devfile);
            *pp_dev = (*pp_dev)->next; // Excluded from list, but NOT freed from memory
        }else{
            pp_dev = &((*pp_dev)->next);
        }
	}
	if(first_dev == NULL){
		error(EXIT_FAILURE, 0, "No devices can be opened.");
	}

	signal(SIGINT, termination_handler);
	signal(SIGQUIT, termination_handler);
	signal(SIGTERM, termination_handler);
	signal(SIGHUP, termination_handler);

	pid = fork();
	switch(pid){
		case -1:
			error(EXIT_FAILURE, errno, "Cannot create new process.\n");
		case 0: // SI reader
			if((datafd = open(FIFO_NAME, O_WRONLY)) == -1){
				error(EXIT_FAILURE, errno, "Cannot open fifo.\n");
			}else{
				si_reader_m(first_dev, datafd, READER_TICK_TIMER);
				close(datafd);
			}
			break;
		default:
			if((datafd = open(FIFO_NAME, O_RDONLY)) == -1){
				error(EXIT_FAILURE, errno, "Cannot open fifo.\n");
			}else{
				curl = curl_easy_init();
				headers = NULL;
				headers = curl_slist_append(headers, "Content-Type: application/json");
				headers = curl_slist_append(headers, "charsets: utf-8");
				do{
					r = read(datafd, &data, sizeof(data));
					if(r > 0){
						if((postdata = si_data_json(&data))){
							curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
							curl_easy_setopt(curl, CURLOPT_URL, "http://localhost/gorgon/www/reader/reader");
							curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
							printf(">>> JSON DATA: %s\n", postdata); 
							curl_easy_perform(curl);
							free(postdata);
						}
					}
				}while(r > 0);
				curl_slist_free_all(headers);
			}
			wait(NULL);
	}

// SI device reset to original state procedure
    dev = first_dev;
    while(dev != NULL){
        if(si_station_resetprot(dev->fd, dev->prot) < 0){
            error(EXIT_SUCCESS, 0, "Reset SI station setup failed for %s.", dev->devfile);
        }
        dev = dev->next;
    }
    return 0;
}
    
