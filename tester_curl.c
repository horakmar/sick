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


#define URL1 "http://"
#define URL2 "/gorgon/www/reader/reader/"

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
"Program cte SI data z logu odesila je na server do aplikace Gorgon\n"
"\n"
"Parametry:\n"
"    -h  ... help - tato napoveda\n"
"    -v  ... verbose - vypisovat vice informaci\n"
"    -q  ... quiet - vypisovat mene informaci\n"
"    -s <server> ... Gorgon <server> [localhost]\n"
"    -r <zavod>  ... Gorgon <zavod> [test]\n"
"    -f <soubor>  ... cist data ze souboru <soubor> [log]\n"
"    -o  ... opakovat cteni souboru stale dokola\n"
"\n"
"Chyby:\n"
"\n";
    printf(help, progname);
    return 0;
}
/* End of usage */

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
/* End of server listener test */

/*******************************
 * main()
 ******************************/

int main(int argc, char **argv){
    int r, dummy_c, f_logfileopenerr;
    struct s_sidata data;
    char *postdata;
    CURL *curl;
    struct curl_slist *headers;
    char *displaystr;
    FILE *fp_data;

#define MAX_READBUF 1024
    char readbuf[MAX_READBUF], *rp;
    
#define MAX_PARAMS 256

/* Loop counters */
    size_t i = 0;
    size_t j = 0;
    size_t p = 0;

#define MAXL_SERVER 51
#define MAXL_RACE 51
#define MAXL_LOGFILE 256

/* Variables */
    int verbose = 1,
	   	loop = 0;
    char server[MAXL_SERVER]; strcpy(server, "localhost");
    char race[MAXL_RACE]; strcpy(race, "test");
    char logfile[MAXL_LOGFILE]; strcpy(logfile, "log");
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
                  case 'o':
                      loop = 1;
                      break;
                  case 's':
                      stringparam(server, MAXL_SERVER);
                      goto nextparm;
                  case 'r':
                      stringparam(race, MAXL_RACE);
                      goto nextparm;
                 case 'f':
                      stringparam(logfile, MAXL_LOGFILE);
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

    if((r = ServerTest(url)) != STAT_OK){
        error(EXIT_FAILURE, 0, "Server error: %s\n", si_stat_errdesc(r));
    }

    if((fp_data = fopen(logfile, "r")) == NULL){
        error(EXIT_FAILURE, errno, "Cannot open log %s", logfile);
    }
	f_logfileopenerr = 0;

    if(verbose > 0) puts("SI reader emulator. Press Enter to send data to server.");

    curl = curl_easy_init();
    headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charsets: utf-8");
    while(! feof(fp_data) && f_logfileopenerr == 0){
        rp = fgets(readbuf, MAX_READBUF, fp_data);
        if(si_json_data(&data, readbuf) == NULL) {
            continue;
        }
		if(feof(fp_data) && loop) {
			fclose(fp_data);
			if((fp_data = fopen(logfile, "r")) == NULL){
				f_logfileopenerr = 1;
				break;
			}
			continue;
		}
        dummy_c = getchar();
        if(rp != NULL){
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
    }
    curl_slist_free_all(headers);

    return 0;
}
    
