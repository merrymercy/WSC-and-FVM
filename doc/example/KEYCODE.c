/****************************************************/
/* File: KEYCODE.c                                  */
/* a sample to show how to get key code             */
/****************************************************/

#include <fxlib.h>
#include <stdio.h>

int main()
{
  int key;

  printf( "PRESS A KEY\n" );

  while( 1 )
  {
    key = waitkey();

    printf( "CODE : %d\n", key ); 
  }
}