/****************************************************/
/* File: wsc.c		                                */
/* main function of WSC complier                    */
/****************************************************/
#include "globals.h"
#include "scanner.h"
#include "prepro.h"
#include "parser.h"
#include "opt.h"
#include "symtab.h"
#include "analyzer.h"
#include "lib.h"
#include "cgen.h"
#include "wsc.h"
#include "file.h"

char *loadFile( const char *name );
void storeFile( const char *name, unsigned char* codes, byteCodeInfo *info );

/* compile a source file 
 * return 0 for success, others for failure
 */
int compile( const char *filename )
{
	char *src;
	unsigned char codes[10240] = {0}, *data = NULL;
	treeNode *root = NULL;
	byteCodeInfo info = {0};

	printf( "WSC  v1.3\nCopyright (C) Wudy 2012\ncompiling...\n", 0xe59e  );

	src = loadFile( filename );
	src = preprocessor( src, &info );

	addLib();

/***************** PARSE *****************/
	if( errorOccurred == FALSE )
		root = parse( src );
	if( src != NULL )
		free( src );
/************* FOLD CONSTANT *************/
	if( errorOccurred == FALSE )
		foldConstant( root );
/*********** BUILD SYBTOLTABLE ***********/
	if( errorOccurred == FALSE )
		data = buildSymtab( root, &info );
/************** CHECK  TYPE **************/
	if( errorOccurred == FALSE )
		typeCheck( root );
/************* FOLD CONSTANT *************/
	if( errorOccurred == FALSE )
		foldConstant( root );
/************* GENERATE CODE *************/
	if( errorOccurred == FALSE )
		codeGen( root, codes, data, &info );
	if( errorOccurred == FALSE )
	{
		char name[50];

		strncpy( name, filename, strlen(filename)-1 );
		name[strlen(filename)-1] = '\0';
		strcat( name, "f" );
		storeFile( name, codes, &info );
	}
/***************** FREE *****************/
	if( data != NULL )
		free( data );
//	freeTree( root );
// I meet a problem, if I don't free tree, i can avoid this problem. so i choose to let
// 9860 OS helo me to free the memory. if you have much experience to deal with such problem,
// contact me(wude.f89@gmail.com)
	freeTypeInfo();
	freeSymtab();
	return 0;
}

/* string table */
const char *tokenToStr[] = {
	"EOF", "ERROR",
	"if", "else", "while", "for", "return", "break",
	"void", "int", "char", "float",
	tokenString, tokenString, tokenString, tokenString, tokenString,
	"+", "-", "*", "/", "%",
	"+=", "-=", "*=", "/=", "%=",
	"<", "<=", ">", ">=", "==", "!=",
	"&&", "||", "!",
	"&",
	"=", ";", ",",
	"(", ")", "[", "]", "{", "}"
};
const char *typeToStr[] = 
{
	"int", "char", "float", "pointer", "array",
	"overload",
	"function", "Lib", "error", "void",
};

/* load file to buffer */
char *loadFile( const char *name )
{
	int handle;
	FONTCHARACTER filename[50];
	int size;
	char *buffer;


	CharToFont( name, filename );
	handle = Bfile_OpenFile( filename, _OPENMODE_READ_SHARE );
	if( handle < 0 )
	{
		throwError( -1, "Can't open src file.\n" );
		return NULL;
	}

	size = Bfile_GetFileSize( handle );

	buffer = (char *)malloc( size * sizeof(char) + 2 );
	if( buffer == NULL )
	{
		outOfMem( -1, "load file" );
		return NULL;
	}

	Bfile_ReadFile( handle, buffer, size, 0 );
	buffer[size] = '\0';
	strcat( buffer, " " ); // for some reasons about the preprocessor...

	Bfile_CloseFile( handle );
	return buffer;
}

/* store bytecode into file */
void storeFile( const char *name, unsigned char* codes, byteCodeInfo *BCInfo )
{
	int handle;
	FONTCHARACTER filename[50];
	int r;

	/* disable, just for call "Bfile_FindFirst" */
	FONTCHARACTER buffer[50];
	FILE_INFO info;
	/* end */


	CharToFont( name, filename );
	r = Bfile_FindFirst( filename, &handle, buffer, &info );
	if( r == 0 )  //already existed, delete it
	{
		r = Bfile_DeleteFile( filename );
		if( r < 0 )
		{
			throwError( -1, "Can't delete the old codes file, try to delete it manually" );
			return;
		}
	}

	handle = Bfile_CreateFile( filename, BCInfo->codeSize );
	if( handle < 0 )
	{
		throwError( -1, "Can't create codes file." );
		return;
	}

	handle = Bfile_OpenFile( filename, _OPENMODE_WRITE );
	if( handle < 0 )
	{
		throwError( -1, "Can't open codes file." );
		return;
	}

	Bfile_WriteFile( handle, codes, BCInfo->codeSize );
	Bfile_CloseFile( handle );
}

/* error handler */
void outOfMem( int line, const char *msg )
{
	throwError( line, "Out of memory!(%s)", msg );
	while( 1 )
		WaitKey();
}

#include <stdarg.h>
char errorOccurred = FALSE;
void throwError( int line, const char *format, ... )
{
	char errorMsg[256];
	char *p = errorMsg;
	va_list ArgList;

	va_start(ArgList, format);
	vsprintf(errorMsg, format, ArgList);
	va_end(ArgList);

	printf( "LINE %3d: %s\n", line, errorMsg );
	errorOccurred = TRUE;

	printf( "--PRESS TO CONTINUE--\n" );
	WaitKey();
}