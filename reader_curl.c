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

#include <curl/curl.h>


#define FIFO_NAME "/tmp/si_data_fifo"
#define READER_TICK_TIMER 60            // seconds

#define URL1 "http://"
#define URL2 "/gorgon/www/reader/reader/"

void termination_handler(int signum){
    f_term = 1;
}

#define stringparam(STRING, MAXSIZE) \
    if(++i > argc) { \
        error(EXIT_FAILURE, ERR_ARGS, "Unset parameter.\n"); \
    } else if(strlen(argv[i]) >= (MAXSIZE)){ \
        error(EXIT_FAILURE, ERR_ARGS, "Parameter too long.\n"); \
    } else { \
        strcpy((STRING), argv[i]); \
    }

/* Usage */
int Usage(char *progname){
	const char help[] = "\n"
"Pouziti:\n"
"    %s [-h] [-tvq]\n"
"\n"
"Program cte SI cipy a odesila data na server do aplikace Gorgon\n"
"\n"
"Parametry:\n"
"    -h  ... help - tato napoveda\n"
"    -v  ... verbose - vypisovat vice informaci\n"
"    -q  ... quiet - vypisovat mene informaci\n"
"    -s <server> ... Gorgon <server> [localhost]\n"
"    -r <zavod>  ... Gorgon <zavod> [test]\n"
"\n"
"Chyby:\n"
"\n";
	printf(help, progname);
	return 0;
}
/* End of usage */

/*
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}
*/

/* Curl callbacks */
size_t debug_cb(char *ptr, size_t size, size_t nmemb, void *dummy){
    size_t realsize = size * nmemb;
	
	printf("Received from server:\n%s\n", ptr);
	return realsize;
}

size_t status_cb(char *ptr, size_t size, size_t nmemb, void *server_status){
    size_t realsize = size * nmemb;
	int *st = (int *) server_status;

	*st = si_getstatus_json(ptr);
	if(*st == STAT_ERROR){
		printf("Bad response from server:\n%s\n", ptr);
	}
	return realsize;
}

size_t display_cb(char *ptr, size_t size, size_t nmemb, char **displaystr){
    size_t realsize = size * nmemb;

	*displaystr = si_getstring_json(ptr, "display_out");
	if(*displaystr == NULL){
		printf("Bad response from server:\n%s\n", ptr);
	}
	return realsize;
}
/* Server listener test */
int ServerTest(char *url){
	CURL *curl;
	char *postdata;
	struct curl_slist *headers;
	int server_status = STAT_NOCONN;

	curl = curl_easy_init();
	headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "charsets: utf-8");
	if((postdata = si_init_json())){
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &server_status);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, status_cb);
		curl_easy_perform(curl);
		free(postdata);
	}
	return server_status;
}
/* End of server listener test*/

int main(int argc, char **argv){
    int datafd;
    int r;
    pid_t pid;
    struct s_sidata data;
    struct s_dev *first_dev, *dev, **pp_dev;
    char *postdata;
    CURL *curl;
    struct curl_slist *headers;
	char *displaystr;
	
#define MAX_PARAMS 256

/* Loop counters */
    size_t i = 0;
    size_t j = 0;
    size_t p = 0;

#define MAXL_SERVER 51
#define MAXL_RACE 51
/* Variables */
    int verbose = 1;
    char server[MAXL_SERVER]; strcpy(server, "localhost");
    char race[MAXL_RACE]; strcpy(race, "test");
    char *url;

    char *params[MAX_PARAMS];

/*
 * Read through command-line arguments for options.
 */
    for(i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            for(j=0; argv[i][j] != '\0'; j++) {
                switch(argv[i][j]) {
                  case 'v':
                      verbose++;
                      break;
                  case 'q':
                      verbose--;
                      break;
                  case 's':
                      stringparam(server, MAXL_SERVER);
                      goto nextparm;
                  case 'r':
                      stringparam(race, MAXL_RACE);
                      goto nextparm;
				  case 'h':
					  Usage(argv[0]);
					  return 0;
                }
            }
          nextparm:
            ;
        }else{
            if(p >= MAX_PARAMS) continue;
            params[p] = malloc(sizeof(*argv[i]));
            strcpy(params[p], argv[i]);
            p++;
        }
    }

/* End of getparam */

    si_verbose = verbose;     // set si library verbose level
    url = (char *) malloc(strlen(URL1) + strlen(URL2) + strlen(server) + strlen(race));
    strcpy(url, URL1);
    strcat(url, server);
    strcat(url, URL2);
    strcat(url, race);
    
    if(verbose > 1) printf(">> URL: %s\n", url);

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

	if((r = ServerTest(url)) != STAT_OK){
		error(EXIT_FAILURE, 0, "Server error: %s\n", si_stat_errdesc(r));
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
			if(verbose > 0) puts("Reading SI cards. Press Ctrl-C to break.");
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
                            curl_easy_setopt(curl, CURLOPT_URL, url);
                            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
							curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &displaystr);
							curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, display_cb);
							//curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, debug_cb);
                            if(verbose > 1) printf(">>> JSON DATA: %s\n", postdata); 
                            curl_easy_perform(curl);
							if(displaystr != NULL){
								puts(displaystr);
								free(displaystr);
							}
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
    
