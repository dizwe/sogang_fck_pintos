/* sum.c */
#include <stdio.h>
#include <string.h>

int fibonacci(int n);
int sum_of_four_int(int a, int b, int c, int d);
int string_to_int(char *s);
int 
main(int argc, char** argv) {
	int arg1, arg2, arg3, arg4;
	arg1 = string_to_int(argv[1]);
	arg2 = string_to_int(argv[2]);
	arg3 = string_to_int(argv[3]);
	arg4 = string_to_int(argv[4]);

	printf("%d %d\n", fibonacci(arg1), sum_of_four_int(arg1, arg2, arg3, arg4));

	return 0;
}

//$$$$

int fibonacci(int n) {
	int a = 0, b = 1, f, i;
	if (n == 0) return a;
	if (n == 1) return b;
	for (i = 2; i <= n; i++) {
		f = a + b;
		a = b;
		b = f;
	}
	return f;
}

int sum_of_four_int(int a, int b, int c, int d) {
	return (a + b + c + d);
}

int string_to_int(char *s){
	int value=0, length, i;
	length = strlen(s);
	
	for(i = 0; i < length; i++){
		value = value * 10 + (s[i] - '0');
	}
	return value;
}
