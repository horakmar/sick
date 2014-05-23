#include <dirent.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <string.h> 
#include <err.h> 

#define SI_DEVICES_MAX 4

#define SI_DEV_PREFIX "/dev/"
#define SI_SEARCHDIR "/sys/devices"
#define SI_DEV_PATTERN "ttyUSB"
#define SI_VENDOR_ID "10c4"
#define SI_PRODUCT_ID "800a"

static int walk(char result[][PATH_MAX], int j){
    int    SI_ID_LEN = strlen(SI_VENDOR_ID);
    int    SI_DEV_PATTERN_LEN = strlen(SI_DEV_PATTERN);
	DIR    *dir;
	struct dirent *d;
	FILE   *fi_idv, *fi_idp; 
	char   idv[SI_ID_LEN], idp[SI_ID_LEN];

    if(j >= SI_DEVICES_MAX) return j;
	if((dir = opendir(".")) == NULL){
		warn("Cannot opendir.");
		return j;
	}
	while((d = readdir(dir))){
		if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0){
			continue;
		}
		if(d->d_type == DT_DIR){
			if(chdir(d->d_name) == 0){
    			j = walk(result, j);
                chdir("..");
            }else{
                warn("Cannot open directory %s", d->d_name);
            }
		}
		if(strncmp(SI_DEV_PATTERN, d->d_name, SI_DEV_PATTERN_LEN) == 0){
			if((fi_idv = fopen("../idVendor", "r")) != NULL){
				if((fi_idp = fopen("../idProduct", "r")) != NULL){
					if(fread(idv, SI_ID_LEN, 1, fi_idv) > 0 &&
					strncmp(idv, SI_VENDOR_ID, SI_ID_LEN) == 0 &&
					fread(idp, SI_ID_LEN, 1, fi_idp) > 0 &&
					strncmp(idp, SI_PRODUCT_ID, SI_ID_LEN) == 0){
						strcpy(result[j++], d->d_name);
					}
					fclose(fi_idp);
				}
				fclose(fi_idv);
			}
		}
	}
	closedir(dir);
	return j;
}

int si_detect_devices(char result[][PATH_MAX]){
	int i, j = 0;
    char filename[PATH_MAX];

	if(chdir(SI_SEARCHDIR) != 0){
		warn("Cannot open directory %s", SI_SEARCHDIR);
		return 0;
	}
	j = walk(result, j);
    for(i = 0; i < j; i++){
        strcpy(filename, result[i]);
        strcpy(result[i], SI_DEV_PREFIX);
        strcat(result[i], filename);
    }
	return j;
}


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
