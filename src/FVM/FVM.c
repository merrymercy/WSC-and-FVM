#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <mathf.h>
#include <revolution.h>
#include "file.h"
#include "fvm.h"
#include "fxlib.h"

#define TRUE  1
#define FALSE 0

typedef struct
{
	char *RAM;
	int RAM_SIZE;
	char *BC;
	int bytecodeSize;
	int dataSize;
	int R[R_SIZE];
	int Rf[Rf_SIZE];
	char *msg;
	char forceBreak;
} virtualEnv;


void throwError( const char *format, ... );
char errorOccurred = FALSE;

/* memory pointer */
unsigned char *RAM = NULL;
int RAM_SIZE;

/* byteCode */
unsigned char *BC;
int bytecodeSize;
int dataSize;

/* registers */
int R[R_SIZE];
float Rf[Rf_SIZE];

/* message between bytecode files */
char *msg;

/* flag */
char forceBreak; // hold [F1] + [F2] + [F6]
char callDepth = 0;

void initEnv( void );
void loadFile( const char *name );
int printInstruction( int pos );
stepResult stepFVM( void );

int temp;
int temp2;
int temp3;
float tempf;

int fvm( const char *filename, const char *message )
{
	int r = srOKAY;

	loadFile( filename );

	if( errorOccurred == FALSE )
		initEnv();

	msg = message;

	while( r == srOKAY )
	{
		if( forceBreak && IsKeyDown(KEY_CTRL_F1) && IsKeyDown(KEY_CTRL_F2)
			&& IsKeyDown(KEY_CTRL_F6) )
		{
			PopUpWin( 4 );
			locate( 9, 3 ); Print( "Break" );
			locate( 6, 5 );  Print( "Press:[EXIT]" );

			while( WaitKey() != KEY_CTRL_EXIT )
				;

			free( RAM );
			free( BC );
			
			return 1;
		}

		r = stepFVM();
	}

	free( RAM );
	free( BC );

	WaitKey();

	return 0;
}

void initEnv( void )
{
	R[$PC] = 0;
	R[$TP] = RAM_SIZE - 10;

	if( RAM == NULL )
		RAM = (unsigned char *)malloc( RAM_SIZE );

	if( RAM == NULL )
		throwError( "No enough memory" );
	else
		memset( RAM, 0, RAM_SIZE );

	memcpy( RAM, BC + bytecodeSize, dataSize );
}

void loadFile( const char *name )
{
	int handle;
	FONTCHARACTER filename[50];
	unsigned char header[HEADER_SIZE];

	CharToFont( name, filename );
	handle = Bfile_OpenFile( filename, _OPENMODE_READ_SHARE );
	if( handle < 0 )
		{ throwError("Can't open file"); return; }

	Bfile_ReadFile( handle, header, HEADER_SIZE, 0 );

	if( header[0] != APP_MAGIC1 || header[1] != APP_MAGIC2 )
		{ throwError("Wrong magic"); Bfile_CloseFile( handle ); return; }
	memcpy( &RAM_SIZE, header+2, sizeof(int) );
	memcpy( &dataSize, header+6, sizeof(int) );
	if( header[13] )
		BigFont();
	forceBreak = header[14];

	bytecodeSize = Bfile_GetFileSize( handle ) - HEADER_SIZE - dataSize;

	if( bytecodeSize < 0 )
		{ throwError("Bad file"); Bfile_CloseFile( handle ); return; }

	BC = (unsigned char*)malloc( bytecodeSize + dataSize );
	if( BC == NULL )
	{ throwError( "No enough memory" ); Bfile_CloseFile( handle ); return; }

	Bfile_ReadFile( handle, BC + bytecodeSize, dataSize, HEADER_SIZE );
	Bfile_ReadFile( handle, BC, bytecodeSize, HEADER_SIZE + dataSize );

	Bfile_CloseFile( handle );
}

int arg[4];
float argf[2];

#define RTC_YEAR            1
#define RTC_MONTH           2
#define RTC_DAY             3
#define RTC_HOUR            4
#define RTC_MINUTE          5
#define RTC_SECOND          6
#define RTC_DAYOFWEEK       7

// sort by frequency
void interrupt( unsigned char vector, unsigned char funcCode )
{
	switch( vector )
	{
		case INT_IO:
			switch( funcCode )
			{
				case INT_IO_PRINT:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					R[0] = printf( RAM+temp );
					break;
				case INT_IO_PRINT_D:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					R[0] = printf( RAM+arg[0], arg[1] );
					break;
				case INT_IO_PRINT_F:
					memcpy( &temp,  RAM+R[$IA],   sizeof(int) );
					memcpy( &tempf, RAM+R[$IA]+4, sizeof(float) );
					R[0] = printf( RAM+temp, tempf );
					break;
				case INT_IO_PRINT_S:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					R[0] = printf( RAM+arg[0], RAM+arg[1] );
					break;
				case INT_IO_SCAN:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					{
						char buffer[48];
						int val;

						getsn( buffer, 48 );
						R[0] = sscanf( buffer, RAM+arg[0], &val );
						memcpy( RAM+arg[1], &val, sizeof(int) );
					}
					break;
				case INT_IO_SCAN_S:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					{
						char buffer[48];

						getsn( buffer, 48 );
						R[0] = sscanf( buffer, RAM+arg[0], RAM+arg[1] );
					}
					break;
				case INT_IO_GETSN:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					R[0] = getsn( RAM+arg[0], arg[1] );
					break;
				case INT_IO_SPRINT:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					R[0] = sprintf( RAM+arg[0], RAM+arg[1] );
					break;
				case INT_IO_SPRINT_D:
					memcpy( arg, RAM+R[$IA], sizeof(int)*3 );
					R[0] = sprintf( RAM+arg[0], RAM+arg[1], arg[2] );
					break;
				case INT_IO_SPRINT_F:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					memcpy( &tempf, RAM+R[$IA]+8, sizeof(float) );
					R[0] = sprintf( RAM+arg[0], RAM+arg[1], tempf );
					break;
				case INT_IO_SPRINT_S:
					memcpy( arg, RAM+R[$IA], sizeof(int)*3 );
					R[0] = sprintf( RAM+arg[0], RAM+arg[1], RAM+arg[2] );
					break;
				case INT_IO_SSCAN:
					memcpy( arg, RAM+R[$IA], sizeof(int)*3 );
					{
						int val;

						R[0] = sscanf( RAM+arg[0], RAM+arg[1], &val );
						memcpy( RAM + arg[2], &val, sizeof(int) );
					}
					break;
				case INT_IO_SSCAN_S:
					memcpy( arg, RAM+R[$IA], sizeof(int)*3 );
					R[0] = sscanf( RAM+arg[0], RAM+arg[1], RAM+arg[2] );
					break;
				case INT_IO_GETCHAR:
					{
						char ch = 0;

						getsn( &ch, 2 );
						R[0] = ch;
					}
					break;
				case INT_IO_PUTCHAR:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					printf( "%c", temp );
					break;
			}
			break;
		case INT_DB:
			switch( funcCode )
			{	
				case INT_DB_ALLCLR:
					Bdisp_AllClr_VRAM();
					break;
				case INT_DB_AREACLR:
					{
						DISPBOX area;

						memcpy( &area.left,  RAM+R[$IA],   sizeof(int) );
						memcpy( &area.top,   RAM+R[$IA]+4, sizeof(int) );
						memcpy( &area.right, RAM+R[$IA]+8, sizeof(int) );
						memcpy( &area.bottom,RAM+R[$IA]+12,sizeof(int) );

						Bdisp_AreaClr_VRAM( &area );
					}
					break;
				case INT_DB_GETDISP:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					Bdisp_GetDisp_VRAM( RAM+temp );
					break;
				case INT_DB_PUTDISP:
					Bdisp_PutDisp_DD();
					break;
				case INT_DB_SETPOINT:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					Bdisp_SetPoint_VRAM( arg[0], arg[1], (char)RAM[R[$IA]+8] );
					break;
				case INT_DB_GETPOINT:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					R[0] = Bdisp_GetPoint_VRAM( arg[0], arg[1] );
					break;
				case INT_DB_DRAWLINE:
					memcpy( arg, RAM+R[$IA], sizeof(int)*4 );
					Bdisp_DrawLineVRAM( arg[0], arg[1], arg[2], arg[3] );
					break;
				case INT_DB_CLEARLINE:
					memcpy( arg, RAM+R[$IA], sizeof(int)*4 );
					Bdisp_ClearLineVRAM( arg[0], arg[1], arg[2], arg[3] );
					break;
				case INT_DB_LOCATE:
					memcpy( arg, RAM+R[$IA], sizeof(int)*2 );
					locate( arg[0], arg[1] );
					break;
				case INT_DB_PRINT:
					memcpy( &temp,  RAM+R[$IA],   sizeof(int) );
					Print( RAM+temp );
					break;
				case INT_DB_PRINTXY:
					memcpy( arg, RAM+R[$IA], sizeof(int)*4 );
					PrintXY( arg[0], arg[1], RAM+arg[2], arg[3] );
					break;
				case INT_DB_PRINTMINI:
					memcpy( arg, RAM+R[$IA], sizeof(int)*4 );
					PrintMini( arg[0], arg[1], RAM+arg[2], arg[3] );
					break;
				case INT_DB_SAVEDISP:
					memcpy( &temp,  RAM+R[$IA], sizeof(int) );
					SaveDisp( temp );
					break;
				case INT_DB_RESTDISP:
					memcpy( &temp,  RAM+R[$IA], sizeof(int) );
					RestoreDisp( temp );
					break;
				case INT_DB_POPUPWIN:
					memcpy( &temp,  RAM+R[$IA], sizeof(int) );
					PopUpWin( temp );
					break;
				case INT_DB_DRAWCIRCLE:
					memcpy( arg, RAM+R[$IA], sizeof(int)*3 );
					{
						register int x, y;
						register int xMax, yMax, doubleR, yInit;

						x = arg[0] - arg[2]; 		if( x < 0 ) x = 0;
						yInit = arg[1] - arg[2];	if( yInit < 0 ) yInit = 0;
						xMax = arg[0] + arg[2];		if( xMax > 127) xMax = 127;
						yMax = arg[1] + arg[2];		if( yMax > 63 ) yMax = 63;

						doubleR = arg[2] * arg[2];

						for( ; x <= xMax; ++x )
						{
							for( y = yInit; y <= yMax; ++y )
							{
								if( abs( (x-arg[0])*(x-arg[0]) + (y-arg[1])*(y-arg[1]) - 
											doubleR ) <  arg[2] )
									Bdisp_SetPoint_VRAM( x, y, 1); 
							}
						}
					}
					break;
				case INT_DB_FILLCIRCLE:
					memcpy( arg, RAM+R[$IA], sizeof(int)*3 );
					{
						register int x, y;
						register int xMax, yMax, doubleR, yInit;

						x = arg[0] - arg[2]; 		if( x < 0 ) x = 0;
						yInit = arg[1] - arg[2];	if( yInit < 0 ) yInit = 0;
						xMax = arg[0] + arg[2];		if( xMax > 127) xMax = 127;
						yMax = arg[1] + arg[2];		if( yMax > 63 ) yMax = 63;

						doubleR = arg[2] * arg[2];

						for( ; x <= xMax; ++x )
						{
							for( y = yInit; y <= yMax; ++y )
							{
								if( abs( (x-arg[0])*(x-arg[0]) + (y-arg[1])*(y-arg[1] ) ) <  doubleR )
									Bdisp_SetPoint_VRAM( x, y, 1); 
							}
						}
					}
					break;
				case INT_DB_DRAWBOX:
					memcpy( arg, RAM+R[$IA], sizeof(int)*4 );
					Bdisp_DrawLineVRAM( arg[0], arg[1], arg[0], arg[3] );
					Bdisp_DrawLineVRAM( arg[0], arg[1], arg[2], arg[1] );
					Bdisp_DrawLineVRAM( arg[2], arg[3], arg[2], arg[1] );
					Bdisp_DrawLineVRAM( arg[2], arg[3], arg[0], arg[3] );
					break;
				case INT_DB_FILLBOX:
					{
						DISPBOX area;

						memcpy( &area.left,  RAM+R[$IA],   sizeof(int) );
						memcpy( &area.top,   RAM+R[$IA]+4, sizeof(int) );
						memcpy( &area.right, RAM+R[$IA]+8, sizeof(int) );
						memcpy( &area.bottom,RAM+R[$IA]+12,sizeof(int) );

						Bdisp_AreaClr_VRAM( &area );
						Bdisp_AreaReverseVRAM( area.left, area.top, area.right, area.bottom );
					}
					break;
				case INT_DB_AREAREV:
					memcpy( arg, RAM+R[$IA], sizeof(int)*4 );
					Bdisp_AreaReverseVRAM( arg[0], arg[1], arg[2], arg[3] );
					break;
			}
			break;
		case INT_CT:
			memcpy( &temp, RAM+R[$IA], sizeof(int) );
			switch( funcCode )
			{
				case INT_CT_ISLOWER:
					R[0] = islower( temp );
					break;
				case INT_CT_ISUPPER:
					R[0] = isupper( temp );
					break;
				case INT_CT_TOLOWER:
					R[0] = tolower( temp );
					break;
				case INT_CT_TOUPPER:
					R[0] = toupper( temp );
					break;
				case INT_CT_ISALNUM:
					R[0] = isalnum( temp );
					break;
				case INT_CT_ISDIGIT:
					R[0] = isdigit( temp );
					break;
				case INT_CT_ISALPHA:
					R[0] = isalpha( temp );
					break;
				case INT_CT_ISGRAPH:
					R[0] = isgraph( temp );
					break;
				case INT_CT_ISPRINT:
					R[0] = isprint( temp );
					break;
				case INT_CT_ISPUNCT:
					R[0] = ispunct( temp );
					break;
				case INT_CT_ISCNTRL:
					R[0] = iscntrl( temp );
					break;
				case INT_CT_ISSPACE:
					R[0] = isspace( temp );
					break;
			}
			break;
		case INT_SL:
			switch( funcCode )
			{
				case INT_SL_ATOF:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					Rf[0] = atof( RAM+temp );
					break;
				case INT_SL_ATOI:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					R[0] = atoi( RAM+temp );
					break;
				case INT_SL_MALLOC:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					R[0] = (int)((unsigned char*)malloc( temp ) - RAM);
					break;
				case INT_SL_FREE:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					free( RAM + temp );
					break;
				case INT_SL_ABS:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					R[0] = abs( temp );
					break;
				case INT_SL_RAND:
					R[0] = rand();
					break;
				case INT_SL_REALLOC:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = (int)((unsigned char*)realloc( RAM+arg[0], arg[1] ) - RAM);
					break;
				case INT_SL_CALLOC:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = (int)((unsigned char*)calloc( arg[0], arg[1] ) - RAM);
					break;
				case INT_SL_SRAND:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					srand( temp );
					break;
			}
			break;
		case INT_STR:
			switch( funcCode )
			{
				case INT_STR_STRCMP:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = strcmp( RAM+arg[0], RAM+arg[1] );
					break;
				case INT_STR_STRCPY:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = (int)(strcpy( RAM+arg[0], RAM+arg[1] ) - RAM);
					break;					
				case INT_STR_STRCAT:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = (int)(strcat( RAM+arg[0], RAM+arg[1] ) - RAM);
					break;
				case INT_STR_STRLEN:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					R[0] = strlen( RAM+temp );
					break;
				case INT_STR_STRCHR:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					temp2 = (char)RAM[R[$IA]+4];
					R[0] = (int)(strchr( RAM+temp, temp2 ) - RAM);
					break;
				case INT_STR_STRSTR:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = (int)(strstr( RAM+arg[0], RAM+arg[1] ) - RAM);
					break;
				case INT_STR_MEMCPY:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*3 );
					memcpy( RAM+arg[0], RAM+arg[1], arg[2] );
					break;
				case INT_STR_MEMSET:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*3 );
					memset( RAM+arg[0], arg[1], arg[2] );
					break;
				case INT_STR_MEMMOVE:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*3 );
					memmove( RAM+arg[0], RAM+arg[1], arg[2] );
					break;
				case INT_STR_STRNCMP:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*3 );
					R[0] = strncmp( RAM+arg[0], RAM+arg[1], arg[2] );
					break;
				case INT_STR_STRNCPY:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*3 );
					R[0] = (int)(strncpy( RAM+arg[0], RAM+arg[1], arg[2] ) - RAM);
					break;				
				case INT_STR_STRNCAT:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*3 );
					R[0] = (int)(strncat( RAM+arg[0], RAM+arg[1], arg[2] ) - RAM);
					break;
				case INT_STR_MEMCMP:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*3 );
					R[0] = memcmp( RAM+arg[0], RAM+arg[1], arg[2] );
					break;
				case INT_STR_MEMCHR:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					temp2 = (char)RAM[R[$IA]+4];
					memcpy( &temp3, RAM+R[$IA]+5, sizeof(int) );
					R[0] = (int)((unsigned char*)memchr( RAM+temp, temp2, temp3 ) - RAM);
					break;
				case INT_STR_STRRCHR:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = (int)(strrchr( RAM+arg[0], arg[1] ) - RAM);
					break;
					break;
				case INT_STR_STRCSPN:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = strcspn( RAM+arg[0], RAM+arg[1] );
					break;
				case INT_STR_STRSPN:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = strspn( RAM+arg[0], RAM+arg[1] );
					break;
				case INT_STR_STRPBRK:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = (int)(strpbrk( RAM+arg[0], RAM+arg[1] ) - RAM);
					break;
			}
			break;
		case INT_KB:
			switch( funcCode )
			{	
				case INT_KB_WAITKEY:
					R[0] = WaitKey();
					break;
				case INT_KB_ISKEYDOWN:
					memcpy( &temp,  RAM+R[$IA], sizeof(int) );
					R[0] = IsKeyDown( temp );
					break;
				case INT_KB_GETKEY:
					{
						unsigned int key;

						memcpy( &temp,  RAM+R[$IA], sizeof(int) );
						R[0] = GetKey( &key );
						temp2 = key;
						memcpy( RAM+temp, &temp2, sizeof(int) );
					}
					break;
			}
			break;
		case INT_FB:
			switch( funcCode )
			{
				case INT_FB_OPENFILE:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					{
						FONTCHARACTER name[50];
	
						CharToFont( RAM+arg[0], name );
						R[0] = Bfile_OpenFile( name, arg[1] );
					}
					break;
				case INT_FB_READFILE:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*4 );
					R[0] = Bfile_ReadFile( arg[0], RAM+arg[1], arg[2], arg[3] );
					break;
				case INT_FB_WRITEFILE:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*3 );
					R[0] = Bfile_WriteFile( arg[0], RAM+arg[1], arg[2] );
					break;
				case INT_FB_SEEKFILE:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = Bfile_SeekFile( arg[0], arg[1] );
					break;
				case INT_FB_CLOSEFILE:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					R[0] = Bfile_CloseFile( temp );
					break;
				case INT_FB_GETFREE:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					{
						int freeSize;

						R[0] = Bfile_GetMediaFree( arg[0], &freeSize );
						memcpy( RAM+arg[1], &freeSize, sizeof(int) );
					}
					break;
				case INT_FB_GETSIZE:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					R[0] = Bfile_GetFileSize( temp );
					break;
				case INT_FB_CREATEFILE:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					{
						FONTCHARACTER name[50];

						CharToFont( RAM+arg[0], name );
						R[0] = Bfile_CreateFile( name, arg[1] );
					}
					break;
				case INT_FB_CREATEDIR:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					{
						FONTCHARACTER name[50];

						CharToFont( RAM+temp, name );
						R[0] = Bfile_CreateDirectory( name );
					}
					break;
				case INT_FB_DELETEFILE:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					{
						FONTCHARACTER name[50];

						CharToFont( RAM+temp, name );
						R[0] = Bfile_DeleteFile( name );
					}
					break;
				case INT_FB_DELETEDIR:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					{
						FONTCHARACTER name[50];

						CharToFont( RAM+temp, name );
						R[0] = Bfile_DeleteDirectory( name );
					}
					break;
				case INT_FB_FINDFIRST:
					memcpy( arg, RAM+R[$IA], sizeof(int)*4 );
					{
						int handle;
						int info[6];
						FILE_INFO	fileinfo;
						FONTCHARACTER pathname[50];
						FONTCHARACTER foundfile[50];

						CharToFont( RAM+arg[0], pathname );
						R[0] = Bfile_FindFirst( pathname, &handle, foundfile, &fileinfo );
						memcpy( RAM+arg[1], &handle, sizeof(int) );
						FontToChar( foundfile, RAM+arg[2] );

						info[0] = fileinfo.id;
						info[1] = fileinfo.type;
						info[2] = fileinfo.fsize;
						info[3] = fileinfo.dsize;
						info[4] = fileinfo.property;
						info[5] = fileinfo.address;

						memcpy( RAM+arg[3], info, sizeof(int)*6 );
					}
					break;
				case INT_FB_FINDNEXT:
					memcpy( arg, RAM+R[$IA], sizeof(int)*3 );
					{
						int info[6];
						FILE_INFO	fileinfo;
						FONTCHARACTER foundfile[50];

						R[0] = Bfile_FindNext( arg[0], foundfile, &fileinfo );
						FontToChar( foundfile, RAM+arg[1] );

						info[0] = fileinfo.id;
						info[1] = fileinfo.type;
						info[2] = fileinfo.fsize;
						info[3] = fileinfo.dsize;
						info[4] = fileinfo.property;
						info[5] = fileinfo.address;

						memcpy( RAM+arg[2], info, sizeof(int)*6 );
					}
					break;
				case INT_FB_FINDCLOSE:
					memcpy( &temp, RAM+R[$IA], sizeof(int) );
					Bfile_FindClose( temp );
					break;
			}
			break;
		case INT_SYS:
			switch( funcCode )
			{
				case INT_SYS_SLEEP:
					memcpy( &temp,  RAM+R[$IA], sizeof(int) );
					Sleep( temp );
					break;
				case INT_SYS_CPUSPEED:
					memcpy( &temp,  RAM+R[$IA], sizeof(int) );
					switch( temp )
					{
						case 1: CPUSpeedNormal();    break;
						case 2: CPUSpeedDouble();    break;
						case 3: CPUSpeedTriple();    break;
						case 4: CPUSpeedQuadruple(); break;
					}
					break;
				case INT_SYS_READRTC:
					memcpy( &temp,  RAM+R[$IA], sizeof(int) );
					switch( temp )
					{
						case RTC_YEAR:   R[0] = RTCReadYear();       break;
						case RTC_MONTH:  R[0] = RTCReadMonth();      break;
						case RTC_DAY:    R[0] = RTCReadDayOfMonth(); break;
						case RTC_HOUR:   R[0] = RTCReadHour();       break;
						case RTC_MINUTE: R[0] = RTCReadMinute();     break;
						case RTC_SECOND: R[0] = RTCReadSecond();     break;
						case RTC_DAYOFWEEK:R[0] = RTCReadDayOfWeek();break;
					}
					break;
				case INT_SYS_SETRTC:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					switch( arg[0] )
					{
						case RTC_YEAR:
							RTCSetYear( arg[1]/1000, arg[1]/100%10, arg[1]/10%10, arg[1]%10 );
							break;
						case RTC_MONTH:
							RTCSetMonth( arg[1]/10, arg[1]%10 );
							break;
						case RTC_DAY:
							RTCSetDayOfMonth( arg[1]/10, arg[1]%10 );
							break;
						case RTC_HOUR:
							RTCSetHour( arg[1]/10, arg[1]%10 );
							break;
						case RTC_MINUTE:
							RTCSetMinute( arg[1]/10, arg[1]%10 );
							break;
						case RTC_SECOND:
							RTCSetSecond( arg[1]/10, arg[1]%10 );
							break;
						case RTC_DAYOFWEEK:
							RTCSetDayOfWeek( arg[1] );
							break;
					}
					break;
				case INT_SYS_BITAND:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = arg[0] & arg[1];
					break;
				case INT_SYS_BITOR:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = arg[0] | arg[1];
					break;
				case INT_SYS_BITXOR:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = arg[0] ^ arg[1];
					break;
				case INT_SYS_BITNOT:
					memcpy( &temp,  RAM+R[$IA], sizeof(int) );
					R[0] = ~temp;
					break;
				case INT_SYS_SHIFTL:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = arg[0] << arg[1];
					break;
				case INT_SYS_SHIFTR:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					R[0] = arg[0] >> arg[1];
					break;
				case INT_SYS_EXEFVM:
					memcpy( arg,  RAM+R[$IA], sizeof(int)*2 );
					{
						virtualEnv env;
						char *filename = (char *)malloc( strlen(RAM+arg[0])+1 );
						char *msg = (char *)malloc( strlen(RAM+arg[1])+1 );

						strcpy( filename, RAM+arg[0] );
						strcpy( msg, RAM+arg[1] );

						// save environment
						env.RAM = RAM; env.RAM_SIZE = RAM_SIZE;
						env.BC = BC; env.bytecodeSize = bytecodeSize; env.dataSize = dataSize;
						memcpy( &env.R, R, sizeof(int) * R_SIZE );
						memcpy( &env.Rf, Rf, sizeof(float) * Rf_SIZE );
						env.msg = msg; env.forceBreak = forceBreak;

						RAM = NULL;

						++callDepth;
						fvm( filename, msg );
						--callDepth;

						env.R[0] = R[0]; // save return value

						// restore environment
						RAM = env.RAM; RAM_SIZE = env.RAM_SIZE;
						BC = env.BC; bytecodeSize = env.bytecodeSize; dataSize = env.dataSize;
						memcpy( R, &env.R, sizeof(int) * R_SIZE );
						memcpy( Rf, &env.Rf, sizeof(float) * Rf_SIZE );
						msg = env.msg; forceBreak = env.forceBreak;

						free( filename );
						free( msg );
					}
					break;
				case INT_SYS_GETFVMMSG:
					if( callDepth )
						R[0] = (int)msg - (int)RAM;
					else
						R[0] = 0;
					break;
				case INT_SYS_RESETCALC:
					Reset_Calc();
					break;
			}
			break;
		case INT_MAF:
			memcpy( &tempf, RAM+R[$IA], sizeof(float) );
			switch( funcCode )
			{
				case INT_MAF_COS:
					Rf[0] = cosf( tempf );
					break;
				case INT_MAF_SIN:
					Rf[0] = sinf( tempf );
					break;
				case INT_MAF_TAN:
					Rf[0] = tanf( tempf );
					break;
				case INT_MAF_POW:
					memcpy( argf,  RAM+R[$IA], sizeof(float)*2 );
					Rf[0] = powf( argf[0], argf[1] );
					break;
				case INT_MAF_SQRT:
					Rf[0] = sqrtf( tempf );
					break;
				case INT_MAF_CEIL:
					Rf[0] = ceilf( tempf );
					break;
				case INT_MAF_FABS:
					Rf[0] = fabsf( tempf );
					break;
				case INT_MAF_FLOOR:
					Rf[0] = floorf( tempf );
					break;
				case INT_MAF_ACOS:
					Rf[0] = acosf( tempf );
					break;
				case INT_MAF_ASIN:
					Rf[0] = asinf( tempf );
					break;
				case INT_MAF_ATAN:
					Rf[0] = atanf( tempf );
					break;
				case INT_MAF_ATAN2:
					memcpy( argf,  RAM+R[$IA], sizeof(float)*2 );
					Rf[0] = atan2f( argf[0], argf[1] );
					break;
				case INT_MAF_COSH:
					Rf[0] = coshf( tempf );
					break;
				case INT_MAF_SINH:
					Rf[0] = sinhf( tempf );
					break;
				case INT_MAF_TANH:
					Rf[0] = tanhf( tempf );
					break;
				case INT_MAF_EXP:
					Rf[0] = expf( tempf );
					break;
				case INT_MAF_LOG:
					Rf[0] = logf( tempf );
					break;
				case INT_MAF_LOG10:
					Rf[0] = log10f( tempf );
					break;
				case INT_MAF_MODF:
					memcpy( &temp,  RAM+R[$IA]+4, sizeof(int) );
					{
						float v;

						Rf[0] = modff( tempf, &v );
						memcpy( RAM+temp, &v, sizeof(float) );
					}
					break;
				case INT_MAF_FMOD:
					memcpy( argf,  RAM+R[$IA], sizeof(float)*2 );
					Rf[0] = fmodf( argf[0], argf[1] );
					break;
			}
			break;
	}
}

// sort by frequency
stepResult stepFVM( void )
{
	switch( BC[R[$PC]] )
	{
		case opLA:
			memcpy( &temp, BC+R[$PC]+2, sizeof(int) );
			R[BC[R[$PC]+1]] = temp + R[BC[R[$PC]+6]];
			R[$PC] += 7;			break;
		case opSTL:
			memcpy( &temp, BC+R[$PC]+2, sizeof(int) );
			memcpy( RAM+temp+R[BC[R[$PC]+6]] , R+BC[R[$PC]+1], sizeof(int) );
			R[$PC] += 7;			break;
		case opLDL:
			memcpy( &temp, BC+R[$PC]+2, sizeof(int) );
			memcpy( &temp, RAM+temp+R[BC[R[$PC]+6]], sizeof(int) );
			memcpy( R+BC[R[$PC]+1], &temp, sizeof(int) );
			R[$PC] += 7;			break;
		case opLCB:
			R[BC[R[$PC]+1]] = (char)BC[R[$PC]+2];
			R[$PC] += 3;			break;		
		case opLCL:
			memcpy( R+BC[R[$PC]+1], BC+R[$PC]+2, sizeof(int) );
			R[$PC] += 6;			break;
		case opLDF:
			memcpy( &temp, BC+R[$PC]+2, sizeof(int) );
			memcpy( &tempf, RAM+temp+R[BC[R[$PC]+6]], sizeof(float) );
			memcpy( Rf+BC[R[$PC]+1], &tempf, sizeof(float) );
			R[$PC] += 7;			break;
		case opLDB:
			memcpy( &temp, BC+R[$PC]+2, sizeof(int) );
			R[BC[R[$PC]+1]] = (char)RAM[temp + R[BC[R[$PC]+6]]];
			R[$PC] += 7;			break;
		case opSTB:
			memcpy( &temp, BC+R[$PC]+2, sizeof(int) );
			RAM[temp + R[BC[R[$PC]+6]]] = R[BC[R[$PC]+1]];
			R[$PC] += 7;			break;
		case opSTF:
			memcpy( &temp, BC+R[$PC]+2, sizeof(int) );
			memcpy( RAM+temp+R[BC[R[$PC]+6]], Rf+BC[R[$PC]+1], sizeof(float) );
			R[$PC] += 7;			break;
		case opLCF:
			memcpy( Rf+BC[R[$PC]+1], BC+R[$PC]+2, sizeof(float) );
			R[$PC] += 6;			break;
		case opCJ:
			{
				char ok;

				switch( BC[R[$PC]+1] )
				{
					case CJ_EQ:
						ok = R[BC[R[$PC]+2]] == 0; break;
					case CJ_NE:
						ok = R[BC[R[$PC]+2]] != 0; break;
					case CJ_LT:
						ok = R[BC[R[$PC]+2]] <  0; break;
					case CJ_LE:
						ok = R[BC[R[$PC]+2]] <= 0; break;
					case CJ_GT:
						ok = R[BC[R[$PC]+2]] >  0; break;
					case CJ_GE:
						ok = R[BC[R[$PC]+2]] >= 0; break;
				}
				if( ok )
				{
					memcpy( &temp, BC+R[$PC]+3, sizeof(int) );
					R[$PC] = temp + R[BC[R[$PC]+7]];
				}
			}
			R[$PC] += 8;			break;
		case opMOV:
			R[BC[R[$PC]+1]] = R[BC[R[$PC]+2]];
			R[$PC] += 3;			break;
		case opADD:
			R[BC[R[$PC]+1]] = R[BC[R[$PC]+2]] + R[BC[R[$PC]+3]];
			R[$PC] += 4;			break;
		case opPUSH:
			memcpy( RAM+R[$TP], R+BC[R[$PC]+1], sizeof(int) );
			R[$TP] -= sizeof(int);
			R[$PC] += 2;			break;
		case opPOP:
			R[$TP] += sizeof(int);
			memcpy( R+BC[R[$PC]+1], RAM+R[$TP], sizeof(int) );
			R[$PC] += 2;			break;
		case opSUB:
			R[BC[R[$PC]+1]] = R[BC[R[$PC]+2]] - R[BC[R[$PC]+3]];
			R[$PC] += 4;			break;
		case opMUL:
			R[BC[R[$PC]+1]] = R[BC[R[$PC]+2]] * R[BC[R[$PC]+3]];
			R[$PC] += 4;			break;
		case opDIV:
			R[BC[R[$PC]+1]] = R[BC[R[$PC]+2]] / R[BC[R[$PC]+3]];
			R[$PC] += 4;			break;
		case opINT:
			interrupt( BC[R[$PC]+1], BC[R[$PC]+2] );
			R[$PC] += 3;			break;
		case opMOVF:
			Rf[BC[R[$PC]+1]] = Rf[BC[R[$PC]+2]];
			R[$PC] += 3;			break;
		case opCJF:
			{
				char ok;

				switch( BC[R[$PC]+1] )
				{
					case CJ_EQ:
						ok = Rf[BC[R[$PC]+2]] == 0; break;
					case CJ_NE:
						ok = Rf[BC[R[$PC]+2]] != 0; break;
					case CJ_LT:
						ok = Rf[BC[R[$PC]+2]] <  0; break;
					case CJ_LE:
						ok = Rf[BC[R[$PC]+2]] <= 0; break;
					case CJ_GT:
						ok = Rf[BC[R[$PC]+2]] >  0; break;
					case CJ_GE:
						ok = Rf[BC[R[$PC]+2]] >= 0; break;
				}
				if( ok )
				{
					memcpy( &temp, BC+R[$PC]+3, sizeof(int) );
					R[$PC] = temp + R[BC[R[$PC]+7]];
				}
			}
			R[$PC] += 8;			break;
		case opFI:
			R[BC[R[$PC]+1]] = (int)Rf[BC[R[$PC]+2]];
			R[$PC] += 3;			break;
		case opIF:
			Rf[BC[R[$PC]+1]] = (float)R[BC[R[$PC]+2]];
			R[$PC] += 3;			break;
		case opFADD:
			Rf[BC[R[$PC]+1]] = Rf[BC[R[$PC]+2]] + Rf[BC[R[$PC]+3]];
			R[$PC] += 4;			break;
		case opFSUB:
			Rf[BC[R[$PC]+1]] = Rf[BC[R[$PC]+2]] - Rf[BC[R[$PC]+3]];
			R[$PC] += 4;			break;
		case opFMUL:
			Rf[BC[R[$PC]+1]] = Rf[BC[R[$PC]+2]] * Rf[BC[R[$PC]+3]];
			R[$PC] += 4;			break;
		case opFDIV:
			Rf[BC[R[$PC]+1]] = Rf[BC[R[$PC]+2]] / Rf[BC[R[$PC]+3]];
			R[$PC] += 4;			break;
		case opPUSHF:
			memcpy( RAM+R[$TP], Rf+BC[R[$PC]+1], sizeof(float) );
			R[$TP] -= sizeof(float);
			R[$PC] += 2;			break;
		case opPOPF:
			R[$TP] += sizeof(float);
			memcpy( Rf+BC[R[$PC]+1], RAM+R[$TP], sizeof(float) );
			R[$PC] += 2;			break;
		case opHALT:
			R[$PC] += 1;			return srHALT;
		case opNOP:
			R[$PC] += 1;			break;
	}
	return srOKAY;
}

void throwError( const char *format, ... )
{
	char ErrorMsg[256];
	va_list ArgList;

	va_start(ArgList, format);
	vsprintf(ErrorMsg, format, ArgList);
	va_end(ArgList);

	printf( ErrorMsg );
	printf( "\n" );
	errorOccurred = TRUE;
}
