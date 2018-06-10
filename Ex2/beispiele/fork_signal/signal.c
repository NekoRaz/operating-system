#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void sig_handler(int signr)
{
	fprintf(stderr, "Speicherzugriffsfehler!\n");
	exit(-1);
}

int main()
{
	int* ptr;
	
	if (signal(SIGSEGV, sig_handler) == SIG_ERR)
		printf("Signalhandler konnte nicht installiert werden\n");
	
	ptr = (int*) 12;
	ptr[1000000] = 3;
	return 0;
}
