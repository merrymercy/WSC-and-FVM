/****************************************************/
/* File: lib.h                                      */
/* library funcion for the WSC compiler             */
/****************************************************/

#ifndef _LIB_H_
#define _LIB_H_

/* add lib function to symbol table */
void addLib( void );

/* load lib function from a header file */
void loadLib( char *name, char **src, int begin );


#define STDIO_OFFSET		0
#define CTYPE_OFFSET		401
#define MATHF_OFFSET		655
#define STDLIB_OFFSET		1077
#define STRING_OFFSET		1288
#define FXLIB_OFFSET		1668

typedef struct
{
	char retType;
	char name[11];
	char vector;
	char funCode;
	char frameSize;
	char args[6];// VAR_VOID marks the end of args
} libFun;

typedef struct
{
	char id[14];
	char replace[6];
} libMacro;

extern libFun WSCLib[];

#endif