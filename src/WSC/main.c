#include "fxlib.h"
#include "file.h"
#include "Wstdio.h"
#include "wsc.h"

extern char errorOccurred;

int AddIn_main( int isAppli, unsigned short OptionNum )
{
	char filename[50];

	SelectFile( filename );
	
	Bdisp_AllClr_DDVRAM();
	
	compile( filename );

	if( errorOccurred )
		printf( "Error." );
	else
		printf( "Done." );

	WaitKey();

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