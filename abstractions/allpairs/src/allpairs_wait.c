#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if(argc < 2)
	return 1;
    char* command = (char *) malloc((strlen("condor_wait   ")+strlen(argv[1]))*sizeof(char));
    sprintf(command,"condor_wait %s",argv[1]); 
    system(command);
    return 0;
}
