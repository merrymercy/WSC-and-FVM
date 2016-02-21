#include "fxlib.h"
#include "fvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fvm( const char *filename, const char *message );

int AddIn_main(int isAppli, unsigned short OptionNum)
{
	char filename[50];

	SelectFile( filename );

	Bdisp_AllClr_DDVRAM();
	fvm( filename, "" );

	return 1;
}


#pragma section _BR_Size
unsigned long BR_Size;
#pragma section
#pragma section _TOP
int InitializeSystem(int isAppli, unsigned short OptionNum)
{
    return INIT_ADDIN_APPLICATION(isAppli, OptionNum);
}
#pragma section