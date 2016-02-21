/****************************************************/
/* File: Wstdio.c                                   */
/* simulate standard input/output                   */
/****************************************************/

/*	                          LISENCE
 *   Copyright (C) 2012  Wudy
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "fxlib.h"
#include "Wstdio.h"
#include <stdarg.h>
#include <string.h>

#define SCREEN_X_MAX 127
#define SCREEN_Y_MAX 63

#define X_MAX 122
#define Y_MAX 58

#define TRUE  1
#define FALSE 0

#ifdef MINI_MODE
#define HEIGHT 6
#else
#define HEIGHT 8
#endif

static int x = 0;
static int y = 0;
static char scroled = FALSE;

static int GetWidth( char c )
{
#ifdef MINI_MODE
	if( c == 'M' || c == 'm' || c == 'N' || c == 'Q' || c == 'W' || c == 'w' ||
		c == '*' || c == '@' || c == '#' || c == '$' )
		return 6;
	else if( c == 'K' || c == 'n' || c == 'r' || c == '~' || c == '%' ||
			 c == '&' )
		return 5;
	else if( c == '(' || c == ')' || c == '[' || c == ']' || c == ';' ||
			c == '\'' || c == ',' || c == ':' )
		return 3;
	else if( c == 'i' || c == '!' || c == '|' )
		return 2;
	else if( c == '\n' )
		return 0;
	else
		return 4;
#else
	if( c == '\n' )
		return 0;
	else
		return 6;
#endif
}

static void wPrint( char *s, char PutDD )
{
	char buf[2] = {0, 0};

	while( *s )
	{
		buf[0] = *s;
		if( *s != '\n' )
		{
#ifdef MINI_MODE
			PrintMini( x, y, buf, MINI_OVER );
#else
			PrintXY( x, y, buf, 0 );
#endif
			if( PutDD )
				Bdisp_PutDisp_DD();
		}
		else
		{
			x = 0;y += HEIGHT;
		}

		x += GetWidth( *s );
		if( x > X_MAX )
		{
			x = 0; y += HEIGHT;
		}
		if( y > Y_MAX )
		{
			DISPBOX area = { 0, 0, SCREEN_X_MAX, SCREEN_Y_MAX };
			unsigned char bitmap[(SCREEN_Y_MAX+1)*(SCREEN_X_MAX+1)];
			GRAPHDATA data = { SCREEN_X_MAX, SCREEN_Y_MAX, 0 };
			DISPGRAPH write = { 0, 0, {0, 0, 0},
				IMB_WRITEMODIFY_NORMAL, IMB_WRITEKIND_OVER};
			int i;


			Bdisp_ReadArea_VRAM( &area, bitmap );

			data.pBitmap = bitmap;	
			memcpy( &write.GraphData, &data, sizeof(GRAPHDATA) );
			
			for( i = HEIGHT; i >= 0; --i )
			{
				int j; // for delay
				for( j = 0; j < 100000; ++j )
					;
				Bdisp_AllClr_VRAM();
				write.y = i - HEIGHT;
				Bdisp_WriteGraph_VRAM( &write );
				Bdisp_PutDisp_DD();
			}
			y -= HEIGHT;
			
			scroled = TRUE;
		}
		else
			scroled = FALSE;
		++s;
	}
}

int printf(const char * format, ... )
{
	char buf[128];
	int length;
	va_list arg_list;

	va_start( arg_list, format );
	length = vsprintf( buf, format, arg_list );
	va_end( arg_list );

	wPrint( buf, TRUE );

	return length;
}

unsigned int WaitKey ( void )
{
	unsigned int key;
	GetKey(&key);
	return key;
}
