/****************************************************/
/* File: CLOCK.c                                    */
/* a sample to show how to read and write RTC       */
/* press F1 to set time                             */
/****************************************************/

#include <stdio.h>
#include <fxlib.h>
#include <math.h>

#define PI 3.1415925635

void main()
{
  while( 1 )
  {
    allclr();
    drawback();
    drawpointer();
    putdisp();
    if( iskeydown( KEY_EXIT ) )
      return;
    if( iskeydown( KEY_F1) )
      settime();
    sleep( 200 );
  }
}

void drawpointer()
{
  float a_hour, a_min, a_sec;
  int x_hour, y_hour, x_min, y_min, x_sec, y_sec;

  a_sec = readrtc( RTC_SECOND ) * 2 * PI / 60;
  a_min = readrtc( RTC_MINUTE ) * 2 * PI / 60 + a_sec / 60;
  a_hour= readrtc( RTC_HOUR   ) * 2 * PI / 12 + a_min / 12;

  x_sec = 63 + 27 * sin(a_sec);
  x_min = 63 + 20 * sin(a_min);
  x_hour= 63 + 13 * sin(a_hour);
  y_sec = 31 - 27 * cos(a_sec);
  y_min = 31 - 20 * cos(a_min);
  y_hour= 31 - 13 * cos(a_hour);

  drawline( 63, 31, x_sec, y_sec );
  drawline( 63, 31, x_min, y_min );
  drawline( 63, 31, x_hour, y_hour );
}

void settime()
{
  int temp;

  allclr();
  printf( "Input year : " );
  scanf( "%d", &temp );
  setrtc( RTC_YEAR, temp );
  printf( "Input month : " );
  scanf( "%d", &temp );
  setrtc( RTC_MONTH, temp );
  printf( "Input day : " );
  scanf( "%d", &temp );
  setrtc( RTC_DAY, temp );
  printf( "Input hour : " );
  scanf( "%d", &temp );
  setrtc( RTC_HOUR, temp );
  printf( "Input minute : " );
  scanf( "%d", &temp );
  setrtc( RTC_MINUTE, temp );
  printf( "Input second : " );
  scanf( "%d", &temp );
  setrtc( RTC_SECOND, temp );

}

void drawback()
{
  int i;
  float angle;

  drawcircle( 63, 31, 10 );
  drawcircle( 63, 31, 31 );
  for( i = 1; i < 13; i += 1 )
  {
    angle = i * 2 * PI / 12;
    setpoint( 63 + 28 * sin(angle), 31 - 27 * cos(angle), 1 );
  }
}

