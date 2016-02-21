/****************************************************/
/* File: FIND.c                                     */
/* a sample to show how to traverse the files       */
/****************************************************/

#include <stdio.h>
#include <fxlib.h>
#include <string.h>

char typetable[11][25];

void init()
{
  strcpy( typetable[0], "Directory" );
  strcpy( typetable[1], "File" );
  strcpy( typetable[2], "Add-In application" );
  strcpy( typetable[3], "eActivity" );
  strcpy( typetable[4], "Language" );
  strcpy( typetable[5], "Bitmap" );
  strcpy( typetable[6], "Main Memory data" );
  strcpy( typetable[7], "Temporary data" );
  strcpy( typetable[8], ". (Current directory)" );
  strcpy( typetable[9], "..(Parent directory)" );
  strcpy( typetable[9], "Volume label" );
}

void printFileInfo( char *filename, int *info ) 
{ 
  char buffer[32];

  allclr();
  sprintf( buffer, "index = %d", info[0] );
  locate( 1, 1 ); print( buffer );
  locate( 1, 2 ); print( filename );
  locate( 1, 3 ); print( typetable[info[1]] );
  sprintf( buffer, "file size : %d", info[2] );
  locate( 1, 4 ); print( buffer );
  sprintf( buffer,"address   : %X", info[5] );
  locate( 1, 5 ); print( buffer );
  locate( 1, 8 ); print( "--PREES TO CONTINUE--" );
}

void traverse()
{
  int info[6]; 
  int handle;
  int key;
  int filename[50];

  if( findfirst( "\\\\fls0\\*", &handle, filename, info ) < 0 )
  {
      locate( 1, 1 ); print( "No file!" );
      waitkey();
      return;
  }

  printFileInfo( filename, info );
  while( findnext( handle, filename, info ) == 0 )
  {
    waitkey();
    printFileInfo( filename, info );
  }

  findclose( handle );  
}

int main()
{
  init();
  traverse();
  return 0;
}
