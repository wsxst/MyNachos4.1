#include "nachos_syscall.h"

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60];
    int i;

    prompt[0] = '-';
    prompt[1] = '-';

    Write("NachOS-SHELL ver 0.0\n", 21 ,output);

    while( 1 )
    {
	Write(prompt, 2, output);

	i = 0;
	
	do {
	
	    Read(&buffer[i], 1, input); 

	} while( buffer[i++] != '\n' );

	buffer[--i] = '\0';
	
	if(strncmp(buffer,"exit", 4)==0) 
		    /*
		     * strncmp has to be replaced with your own 
		     * implementation
		     */
	    Halt(); //stops the Simulation
	
	if( i > 0 ) {
	    newProc = Exec(buffer);
	    Join(newProc);
	}
    }
}

