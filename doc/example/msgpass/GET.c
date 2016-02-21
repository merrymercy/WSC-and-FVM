/****************************************************/
/* File: GET.c                                      */
/* a sample to show how to receive message from the */
/* program that calls me                            */
/****************************************************/

#include <stdio.h>
#include <fxlib.h>

int main()
{
  printf( "receive : %s\n", getfvmmsg() );

  return 99;
}
