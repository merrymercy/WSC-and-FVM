/****************************************************/
/* File: SPRING.c                                   */
/* simulate spring                                  */
/****************************************************/

#include <fxlib.h>

#define BALL_RADIUS 5

void main()
{
  int mx, my, bx, by; // m = mouse, b = ball
  int key;
  float vx, vy;

  mx = bx = 63; my = by = 10;
  vx = vy = 0;

  while( 1 )
  {
    float fx = mx - bx;
    float fy = my - by + 20;

    vx += (fx / 40);
    vy += (fy / 40);

    vx *= 0.98; vy *= 0.98;

    bx += vx; by += vy;

    allclr();
    fillcircle( bx, by, BALL_RADIUS );
    drawmouse( mx, my );
    drawline( mx, my, bx, by );
    putdisp();
    if( iskeydown( KEY_UP ) )
      my -= 3;
    if( iskeydown( KEY_DOWN ) )
      my += 3;
    if( iskeydown( KEY_LEFT ) )
      mx -= 3;
    if( iskeydown( KEY_RIGHT) )
      mx += 3;
    if( iskeydown( KEY_EXIT ) )
      break;
    sleep( 30 );
  }
}

void drawmouse( int x, int y )
{
  drawline( x-1, y, x+1, y );
  drawline( x, y-1, x, y+1 );
}