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

#define TAB_SIZE 25

#define TRUE  1
#define FALSE 0

static int MINI_MODE = TRUE;

static int HEIGHT = 6;

static int x = 0;
static int y = 0;
static char scroled = FALSE;

void BigFont( void )
{
	MINI_MODE = FALSE;
	HEIGHT = 8;
}


static int GetWidth( char c )
{
	if( MINI_MODE )
	{
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
		else if( c == '\n' || c == '\t' )
			return 0;
		else
			return 4;
	}
	else
	{
		if( c == '\n' || c == '\t' )
			return 0;
		else
			return 6;
	}
}

static void wPrint( char *s, char PutDD )
{
	char buf[2] = {0, 0};

	while( *s )
	{
		buf[0] = *s;

		if( *s == '\n' )
		{
			x = 0;y += HEIGHT;		
		}
		else if( *s == '\t' )
		{
			x = (x/TAB_SIZE + 1) * TAB_SIZE;
		}
		else
		{
			if( MINI_MODE )
				PrintMini( x, y, buf, MINI_OVER );
			else
				PrintXY( x, y, buf, 0 );
			if( PutDD )
				Bdisp_PutDisp_DD();		
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

static char KeyToChar( int key )
{
	char ch = 0;

	switch( key )
	{
		case KEY_CHAR_POW:   ch = '^'; break;
		case KEY_CHAR_COMMA: ch = ','; break;
		case KEY_CHAR_LPAR:  ch = '('; break;
		case KEY_CHAR_RPAR:  ch = ')'; break;
		case KEY_CHAR_LBRCKT:ch = '['; break;
		case KEY_CHAR_RBRCKT:ch = ']'; break;
		case KEY_CHAR_LBRACE:ch = '{'; break;
		case KEY_CHAR_RBRACE:ch = '}'; break;
		case KEY_CHAR_PLUS : ch = '+'; break;
		case KEY_CHAR_MINUS: ch = '-'; break;
		case KEY_CHAR_MULT : ch = '*'; break;
		case KEY_CHAR_DIV  : ch = '/'; break;
		case KEY_CHAR_SPACE: ch = ' '; break;
		case KEY_CHAR_DQUATE:ch = '\"';break;
		default:
			if( key>=KEY_CHAR_0 && key<=KEY_CHAR_9 || key == KEY_CHAR_DP )
				ch = key;
			else if( key>=KEY_CHAR_A && key<=KEY_CHAR_Z )
				ch = key + 32;
			else if( key==KEY_CHAR_MINUS || key== KEY_CHAR_PMINUS )
				ch = '-';
			break;
	}

	return ch;
}


char *getsn( char *buffer, int len )
{
	DISPBOX area;  // for Bdisp_AreaClr_VRAM();
	unsigned int key;
	char ch;
	int i = 0;
	int RawX = x, RawY = y;
	char UpperCase = FALSE;

	printf( "_" );

	GetKey( &key );
	while( key != KEY_CTRL_EXE )
	{
		if( ch = KeyToChar( key ) )
		{
			if( i < len - 1 )
				buffer[i++] = (UpperCase ? toupper(ch) : ch);
		}
		else
		{
			if( key == KEY_CTRL_AC )
				i = 0;
			else if( key == KEY_CTRL_DEL )
			{
				if( i > 0 )
					--i;
			}
			else if( key == KEY_CTRL_EXIT || key == KEY_CTRL_QUIT )
			{
				buffer[0] = '\0';

				area.left = RawX; area.top = RawY;
				area.right = SCREEN_X_MAX; area.bottom = y + HEIGHT - 1;
				Bdisp_AreaClr_DDVRAM( &area );

				return NULL;
			}
			else if( key == KEY_CTRL_F6 )
				UpperCase = !UpperCase;
		}
		buffer[i] = '\0';

		area.left = RawX; area.top = RawY;
		area.right = SCREEN_X_MAX; area.bottom = y + HEIGHT - 1;
		Bdisp_AreaClr_VRAM( &area );

		x = RawX; y = RawY;
		wPrint( buffer, FALSE );
		wPrint( "_", FALSE );

		if( scroled )
			RawY -= HEIGHT;

		GetKey( &key );
	}
	area.left = x - GetWidth( '_' ) ; area.top = y;
	area.right = x; area.bottom = y + HEIGHT-1;
	Bdisp_AreaClr_VRAM( &area );
	wPrint( "\n", FALSE );
	return buffer;
}

void ResetIO( void )
{
	x = y = 0;
}

unsigned int WaitKey ( void )
{
	unsigned int key;
	GetKey(&key);
	return key;
}
