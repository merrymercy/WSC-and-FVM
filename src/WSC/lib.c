/****************************************************/
/* File: lib.c                                      */
/* library funcion for the WSC compiler             */
/****************************************************/

#include "globals.h"
#include "scanner.h"
#include "parser.h"
#include "lib.h"
#include "symtab.h"
#include "fvm.h"
#include "file.h"
#include "fxlib.h"

#define MAXLIB 130

libFun WSCLib[MAXLIB];

static int total = 0;

/* add lib function to symbol table */
void addLib( void )
{
	int i;

	for( i = 0; i < total ; ++i )
	{
		Object obj;

		obj.type.t = VAR_LIBFUN;
		obj.type.n = newTypeInfo( WSCLib[i].retType );
		obj.val.fval.address = (treeNode *)&WSCLib[i];
		obj.val.fval.frameSize = WSCLib[i].frameSize;
		symtabInsert( WSCLib[i].name, &obj, LIBFUN_SCOPE, 0, -1 );
	}
}

/* add lib function from bin */
extern char *strrep( char *src, char *oldStr, char *newStr );
void loadLib( char *name, char **src, int begin )
{
	int handle;
	char libCount, defCount;
	int i, offset;
	libMacro WSCDef[100];
	FONTCHARACTER filename[50];


	CharToFont( "\\\\"ROOT"\\WSCLIB", filename );
	
	handle = Bfile_OpenFile( filename, _OPENMODE_READ );
	if( handle < 0 )
	{
		throwError( -1, "Can't open lib" );
		return;
	}

	if( strcmp( name, "stdio.h" ) == 0 )
		offset = STDIO_OFFSET;
	else if( strcmp( name, "stdlib.h" ) == 0 )
		offset = STDLIB_OFFSET;
	else if( strcmp( name, "string.h" ) == 0 )
		offset = STRING_OFFSET;
	else if( strcmp( name, "fxlib.h" ) == 0 )
		offset = FXLIB_OFFSET;
	else if( strcmp( name, "ctype.h" ) == 0 )
		offset = CTYPE_OFFSET;
	else if( strcmp( name, "math.h" ) == 0 )
		offset = MATHF_OFFSET;
	else
		throwError( lineno, "'%s' is not lib", name );
		
	Bfile_SeekFile( handle, offset );

	Bfile_ReadFile( handle, &libCount, sizeof(char), -1 );
	Bfile_ReadFile( handle, &defCount, sizeof(char), -1 );
	Bfile_ReadFile( handle, WSCLib + total, sizeof(libFun)*libCount, -1 );
	Bfile_ReadFile( handle, WSCDef, sizeof(libMacro)*defCount, -1 );

	total += libCount;

	for( i = 0; i < defCount; ++i )
	{
		setScanner( *src, lineno, begin );
		*src = strrep( *src, WSCDef[i].id,  WSCDef[i].replace );
	}

	Bfile_CloseFile( handle );
}