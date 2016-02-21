/****************************************************/
/* File: FILE.c                                     */
/* a sample to show how to use file IO function     */
/****************************************************/

#include <fxlib.h>
#include <string.h>
#include <stdio.h>

int main()
{
  char *msg = "Hello, WSC & FVM!";
  char buffer[30];
  int handle, size;

  // create directory and file
  if( createdir( "\\\\fls0\\test" ) < 0 )
  {
    printf( "Can't create directory!\n" );
    return -1;
  }
  if( createfile( "\\\\fls0\\test\\a.txt", strlen( msg ) ) < 0 )
  {
    printf( "Can't create file!\n" );
    return -1;
  }

  // write file
  if( (handle = openfile( "\\\\fls0\\test\\a.txt" , OPEN_W )) < 0 )
  {
    printf( "Can't write file!\n" );
    return -1;
  }
  writefile( handle, msg, strlen( msg ) );
  closefile( handle );

  // read file
  if( (handle = openfile( "\\\\fls0\\test\\a.txt" , OPEN_R )) < 0 )
  {
  	printf( "Can't read file!\n" );
  	return -1;
  }
  size = getsize( handle );
  readfile( handle, buffer, size, 0 );
  closefile( handle );
  buffer[size] = 0;

  // print text
  printf( buffer );
  return 0;
}