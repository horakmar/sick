/*
 * Commandline parameter parsing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{

#define MAX_PARAMS 256

/* Loop counters */
  size_t i = 0;
  size_t j = 0;
  size_t p = 0;

/* Variables */
  int verbose = 1;
  int test = 0;

  char* params[MAX_PARAMS];

/*
 * Read through command-line arguments for options.
 */
  printf("argc = %d\n\n", argc);
  printf("argv[%u] = %s\n", 0, argv[0]);
  for(i = 1; i < argc; i++){
      printf("argv[%u] = %s\n", i, argv[i]);
      if (argv[i][0] == '-') {
          for(j=0; argv[i][j] != '\0'; j++){
              switch(argv[i][j]){
                  case 'v':
                      verbose++;
                      break;
                  case 'q':
                      verbose--;
                      break;
                  case 't':
                      test = 1;
                      break;
              }
          }
      }else{
          if(p >= MAX_PARAMS) continue;
          params[p] = malloc(sizeof(*argv[i]));
          strcpy(params[p], argv[i]);
          p++;
      }
  }
  printf("******************\nverbose = %d\ntest = %d\n", verbose, test);
  puts("Parameters:");
  for(i = 0; i < p; i++){
      printf("%s\n", params[i]);
  }
}

