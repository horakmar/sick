#include <stdio.h>

int main(void){
	int a = 0;

	do{
		printf("%d\n", a);
		a++;
		if(a > 8 && a < 15){
			continue;
		}
		puts("tick");
	}while(a < 10);
}
