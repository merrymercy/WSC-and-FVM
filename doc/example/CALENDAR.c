 /****************************************************/
/* File: CALENDAR.c                                 */
/* a simple calendar                                */
/****************************************************/

#include <fxlib.h>
#include <string.h>
#include <stdio.h>

int mdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int moff[12] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
char mname[12][10];

void init()
{
  strcpy( mname[0], "January" );
  strcpy( mname[1], "February" );
  strcpy( mname[2], "March" );
  strcpy( mname[3], "April" );
  strcpy( mname[4], "May" );
  strcpy( mname[5], "June" );
  strcpy( mname[6], "July" );
  strcpy( mname[7], "August" );
  strcpy( mname[8], "September" );
  strcpy( mname[9], "October" );
  strcpy( mname[10], "November" );
  strcpy( mname[11], "December" );
}

int getday( int m, int d, int y )
{
  int dayoff, monthoff, yearoff, cenoff, leap;
  dayoff = d % 7;
  monthoff = moff[m-1];

  if( m < 3 && (y % 4 == 0) )
    leap=1;
  else
    leap=0;
  
  yearoff = ( (y%100) + ((y%100)/4) - leap ) % 7;
  cenoff = ( (y/400) * 4 + 3 - (y/100) ) * 2;
  return (cenoff + yearoff + monthoff + dayoff) % 7;
}

void display( int m, int year )
{
  char temp1[20], temp2[10];
  int offset, leap, i, x, y;

  allclr();
  sprintf( temp1, "%s, ", mname[m-1] );
  sprintf( temp2, "%d", year );
  strcat( temp1, temp2 );
  locate( 1, 1 ); print( temp1 );
  locate( 1, 2 ); print( "SU MO TU WE TH FR SA" );

  offset = getday( m, 1, year );

  if( m == 2 && year % 4 == 0 )
    leap = 1;
  else
    leap = 0;

  x = offset; y = 0;
  for( i = 0; i < (mdays[m-1] + leap); i += 1 )
  {
    sprintf( temp1, "%d", i + 1 );
    locate( x*3 + 1, y + 3 );
    print( temp1 );
    x += 1;
    if( x > 6 )
    {
       x=0; y += 1;
    }
  }
}

void main()
{
  int month = 6, year = 2012;
  int key;

  init();
  
  while( 1 )
  {
    display( month, year );
    
    while( 1 )
    {
      getkey( &key );
      if( key == KEY_UP )
      {
        year -= 1; break;
      }
      if( key == KEY_DOWN )
      {
        year += 1; break;
      }
      if( key == KEY_LEFT && month > 1 )
      {
        month -= 1; break;
      }
      if( key == KEY_RIGHT && month < 12 )
      {
        month += 1; break;
      }
    }
  }
}