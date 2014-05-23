#include <dirent.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <string.h> 
#include <err.h> 

#define DEVICES_MAX 4

#define SI_SEARCHDIR "/sys/devices"
#define SI_DEV_PATTERN "ttyUSB"
#define SI_DEV_PATTERN_LEN 6
#define SI_VENDOR_ID "10c4"
#define SI_PRODUCT_ID "800a"
#define SI_ID_SIZE 4

int walk(char result[][PATH_MAX], int *j){
	DIR    *dir;
	struct dirent *d;
	FILE   *fi_idv, *fi_idp; 
	char   idv[SI_ID_SIZE], idp[SI_ID_SIZE] ;

	if((dir = opendir(".")) == NULL){
		warn("Cannot opendir.");
		return -1;
	}
	while((d = readdir(dir)) && *j < DEVICES_MAX){
		if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
			continue;
		}
		if(d->d_type == DT_DIR) {
			chdir(d->d_name);
			walk(result, j);
			chdir( ".." );
		}
		if(strncmp(SI_DEV_PATTERN, d->d_name, SI_DEV_PATTERN_LEN) == 0){
			if((fi_idv = fopen("../idVendor", "r")) != NULL){
				if((fi_idp = fopen("../idProduct", "r")) != NULL){
					if(fread(idv, SI_ID_SIZE, 1, fi_idv) > 0 &&
					strncmp(idv, SI_VENDOR_ID, SI_ID_SIZE) == 0 &&
					fread(idp, SI_ID_SIZE, 1, fi_idp) > 0 &&
					strncmp(idp, SI_PRODUCT_ID, SI_ID_SIZE) == 0){
						strcpy(result[*j], d->d_name);
						(*j)++;
					}
					fclose(fi_idp);
				}
				fclose(fi_idv);
			}
		}
	}
	closedir(dir);
	return 0;
}

int detect(char result[][PATH_MAX]){
	int j = 0;

	if(chdir(SI_SEARCHDIR) != 0){
		warn("Cannot open directory %s", SI_SEARCHDIR);
		return 0;
	}
	walk(result, &j);
	return j;
}


int main(int argc, char* argv[]){
	char devices[DEVICES_MAX][PATH_MAX];
	int num, j;

	num = detect(devices);
	for(j = 0; j < num; j++){
		printf("%d: %s\n", j, devices[j]);
	}
	return 0;
}
