/*
 * Test CRC computation
 */

#include <stdio.h>

int main(int argc, char *argv[]) {
	char *errors[] = {
		"Unknown error",												// 0
		"Cannot open serial device.",									// 1
		"Cannot get serial device attributes.",
		"Cannot set serial device attributes.",
		"No start data found.",
		"No memory left for operation.",
		"Bad CRC."
	};
	int i,j;

	i = sizeof(errors);
	j = sizeof(errors) / sizeof(errors[0]);

	printf("Size of errors = %d\nElement count = %d\n", i, j);
	return 0;
} 

