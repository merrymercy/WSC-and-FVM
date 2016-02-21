/****************************************************/
/* File: BALL.c                                     */
/* simulate falling ball                            */
/****************************************************/

#include <fxlib.h>
#include <stdlib.h>

#define BALL_RADIUS 5
#define X_MAX       127
#define Y_MAX       63
#define LOST        -0.95

float x, y, vx, vy, ax, ay;

void main()
{
  x  = 10; y  = 10;
  vx = 2;  vy = 0;
  ax = 0;  ay = 0.2;

  while( 1 )
  {
    allclr();
    fillcircle( x, y, BALL_RADIUS );    
    if( iskeydown( KEY_UP ) )
    {
      locate( 1, 1 ); print( "KEY : UP" );
      vy -= 0.5;
    }
    if( iskeydown( KEY_DOWN ) )
    {
      locate( 1, 1 ); print( "KEY : DOWN" );
      vy += 0.5;
    }
    if( iskeydown( KEY_LEFT ) )
    {
      locate( 1, 1 ); print( "KEY : LEFT" );
      vx -= 0.5;
    }
    if( iskeydown( KEY_RIGHT ) )    
    {
      locate( 1, 1 ); print( "KEY : RIGHT" );
      vx += 0.5;
    }
    putdisp();
    update();
    if( iskeydown( KEY_EXIT ) )
      break;
    sleep( 30 );
  }
}

void update()
{
  vx += ax; vy += ay;
  x += vx*0.75; y += vy*0.75;

  if( x > X_MAX - BALL_RADIUS + 1 )
  {
    x = X_MAX - BALL_RADIUS; vx *= LOST;
  }
  else if( x < BALL_RADIUS - 1 )
  {
    x = BALL_RADIUS; vx *= LOST;
  }

  if( y > Y_MAX - BALL_RADIUS + 1 )
  {
    y = Y_MAX - BALL_RADIUS; vy *= LOST;
  }
  else if( y < BALL_RADIUS - 1 )
  {
    y = BALL_RADIUS; vy *= LOST;
  }
}