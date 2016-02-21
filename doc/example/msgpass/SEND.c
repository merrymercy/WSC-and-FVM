/****************************************************/
/* File: SEND.c                                     */
/* a sample to show how to execute a fvm program    */
/****************************************************/

#include <stdio.h>
#include <fxlib.h>

int main()
{
  int r;

  r = exefvm( "\\\\fls0\\GET.f", "Hello!" );
  printf( "return : %d", r );

  return 0;
}
