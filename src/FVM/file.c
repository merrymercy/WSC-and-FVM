#include "fxlib.h"
#include "file.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FILENAMEMAX 13
#define N_LINE 6

typedef struct{
	char filename[FILENAMEMAX];
	unsigned long filesize;
}Files;

static Files *files;
static int index = 0;

static int ReadFile( char *folder);
static int IsFileNeeded( FONTCHARACTER *FileName );
static FONTCHARACTER *FilePath(char *sFolder, FONTCHARACTER *sFont);
static void Explorer( int size, char *folder );
static int FileCmp( const void *p1, const void *p2 );
static void DrawBar( void );

char *SelectFile (char *filename)
{
	char folder[FILENAMEMAX] = "", name[FILENAMEMAX];
	int size;
	
	
	Bdisp_AllClr_DDVRAM();
	while( 1 ){
		size = ReadFile( folder );
		qsort( files, size, sizeof(Files), FileCmp );

		Explorer( size, folder );

		if( index == size ){							//return to root
			folder[0] = '\0';
			index = 0;
		}
		else if( files[index].filesize == -1 ){				//folder
			strcpy( folder,files[index].filename );
			index = 0;
		}
		else{										//file
			strcpy( name,files[index].filename );
			free( files );
			break;
		}
	}

	if( strlen(folder) == 0 )
		sprintf( filename, "\\\\"ROOT"\\%s", name );
	else
		sprintf( filename, "\\\\"ROOT"\\%s\\%s", folder, name );
	
	return filename;
}

static int ReadFile( char *folder )
{
	char str[FILENAMEMAX];
	FONTCHARACTER find_path[50];
	FONTCHARACTER find_name[50];
	FILE_INFO file_info;
	int find_h;
	int size, i;

	size = 0;
	FilePath( folder, find_path );

/*				Get File Num			*/

	Bfile_FindFirst (find_path,&find_h,find_name,&file_info);
	if( file_info.type == DT_DIRECTORY ||  IsFileNeeded( find_name ) )
		size++;
	while(Bfile_FindNext(find_h,find_name,&file_info)==0){
		if( file_info.type == DT_DIRECTORY || IsFileNeeded( find_name ) )
			size++;
	}
	Bfile_FindClose(find_h);
/*				Get Name & Size			*/
	i = 0;
	files = (Files *)malloc(size*sizeof(Files));
	memset( files, 0, size*sizeof(Files));
	Bfile_FindFirst (find_path,&find_h,find_name,&file_info);
	if( file_info.type == DT_DIRECTORY ||  IsFileNeeded( find_name ) ){
		FontToChar(find_name,str);
		strncpy( files[i].filename, str, FILENAMEMAX);
		files[i].filesize = (file_info.type == DT_DIRECTORY ? -1 : file_info.dsize);
		++i;
	}
	while(Bfile_FindNext(find_h,find_name,&file_info)==0)
	{
		if( file_info.type == DT_DIRECTORY ||  IsFileNeeded( find_name ) ){
			FontToChar(find_name,str);
			strncpy( files[i].filename, str, FILENAMEMAX);
			files[i].filesize = (file_info.type == DT_DIRECTORY ? -1 : file_info.dsize);
			++i;
		}
	}
	Bfile_FindClose( find_h );
	return size;
}

static void Explorer( int size, char *folder )
{
	int top, redraw;
	int i;
	unsigned int key;
	
	redraw = 1;
	top = index;
	
	while(1)
	{
		if( redraw )
		{
			Bdisp_AllClr_VRAM();
			DrawBar();
			locate(1, 1);Print("Programs   [        ]");
			locate(13, 1);Print( strlen(folder) ? folder : "Root");
			if( size < 1 ){
				locate( 8, 4 );
				Print( "No Data" );
			}
			else{
				char buf[22];
				if( top > index )
					top = index;
				if( index > top + N_LINE - 1 )
					top = index - N_LINE + 1;
				if( top < 0 )
					top = 0;
	
				for(i = 0;i < N_LINE && i + top < size; ++i ){
					locate( 1, i + 2 );
					if( files[i + top].filesize == -1 )
						sprintf( buf, " [%s]", files[i + top].filename );
					else
						sprintf( buf, " %-12s:%6u ", files[i + top].filename, files[i + top].filesize );
					Print( buf );
				}
				Bdisp_AreaReverseVRAM( 0, (index-top+1)*8 , 127, (index-top+2)*8-1 );
				if( top > 0 )
					PrintXY( 120, 8, "\xE6\x92", top == index );
				if( top + N_LINE < size  )
					PrintXY( 120, N_LINE*8, "\xE6\x93" , top + N_LINE - 1 == index );
			}
			redraw = 0;
		}
		GetKey(&key);
		if( key==KEY_CTRL_UP ){
			if( --index < 0 )
				index = size - 1;
			redraw = 1;
		}
		else if( key==KEY_CTRL_DOWN ){
			if( ++index > size - 1 )
				index = 0;
			redraw = 1;
		}
		else if( key==KEY_CTRL_EXE || key == KEY_CTRL_F1)
			break;		
		else if( key == KEY_CTRL_F2 )
		{
			PopUpWin( 6 );

			locate( 7, 2 ); Print( "FVM v1.30" );
			locate( 12, 3 ); Print( "by Wudy" );
			locate( 4, 5 ); Print( "virtual machine" );
			locate( 4, 6 ); Print( "to run the code" );
			locate( 4, 7 ); Print( "generate by WSC" );

			WaitKey();
			redraw = 1;
		}
		else if( key==KEY_CTRL_EXIT){
			index = size;
			break;
		}
	}
}

static int IsFileNeeded( FONTCHARACTER *FileName )
{
	char str[13];
	FontToChar( FileName, str );
	return (strcmp(str + strlen(str) - 2, ".f") == 0);
}

static int FileCmp( const void *p1, const void *p2 )
{
	Files *f1 = (Files *)p1;
	Files *f2 = (Files *)p2;

	if( f1->filesize == -1 && f2->filesize == -1 )
		return strcmp( f1->filename + 1, f2->filename + 1);
	else if( f1->filesize == -1 )
		return -1;
	else if( f2->filesize == -1 )
		return 1;
	else
		return strcmp( f1->filename, f2->filename );
}

static const char graph_bar[]={
	0xFF,0xFF,0xE7,0xFF,0xFF,
	0x8D,0xB1,0xE4,0x4C,0x6F,
	0xB5,0xAD,0xE6,0xD5,0xD7,
	0x8D,0xAD,0xE6,0xD4,0x57,
	0xAD,0xAD,0xE6,0xD5,0xD7,
	0xB5,0xAD,0xC6,0xD5,0xD6,
	0xB6,0x6D,0x84,0x55,0xEC,
	0xFF,0xFF,0x7,0xFF,0xF8,
};

static void DrawBar( void )
{
    DISPGRAPH dg; 
    dg.x = 1; 
    dg.y = 56; 
    dg.GraphData.width = 40; 
    dg.GraphData.height = 8; 
    dg.GraphData.pBitmap = graph_bar; 
    dg.WriteModify = IMB_WRITEMODIFY_NORMAL; 
    dg.WriteKind = IMB_WRITEKIND_OVER; 
    Bdisp_WriteGraph_DDVRAM(&dg); 
}

FONTCHARACTER * CharToFont( const char *cFileName, FONTCHARACTER *fFileName )
{
	int i,len = strlen(cFileName);
	for(i=0; i<len ;++i)
		fFileName[i] = cFileName[i];
	fFileName[i]=0;
	return fFileName;
}

char * FontToChar( const FONTCHARACTER *fFileName, char *cFileName )
{
	int i = 0;
	while((cFileName[i]=fFileName[i])!=0)++i;
	return cFileName;
}

static FONTCHARACTER *FilePath( char *sFolder, FONTCHARACTER *sFont )
{
	char path[50];

	if( strlen(sFolder)==0 )
		sprintf(path,"\\\\"ROOT"\\*" );
	else
		sprintf(path,"\\\\"ROOT"\\%s\\*",sFolder);

	//Convert to FONTCHARACTER
	CharToFont(path,sFont);
	return(sFont);
}
