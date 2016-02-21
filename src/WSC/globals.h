/****************************************************/
/* File: globals.h                                  */
/* Global types and vars for the WSC compiler       */
/****************************************************/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

extern char errorOccurred;
extern void outOfMem( int line, const char *msg );
extern void throwError( int line, const char *format, ... );

// token type
typedef enum
{
    /* book-keeping tokens */
	TK_EOF,TK_ERROR,
    /* keywords */
    TK_IF, TK_ELSE, TK_WHILE, TK_FOR, TK_RETURN, TK_BREAK,
	TK_VOID, TK_INT, TK_CHAR, TK_FLOAT,
    /* multi-character tokens */
    TK_ID, TK_NUM, TK_NUM_F, TK_NUM_HEX, TK_STR, TK_CH,
    /* special symbols */
    TK_ADD, TK_SUB, TK_MUL, TK_DIV, TK_MOD,      // + - * / %
	TK_ADD_, TK_SUB_, TK_MUL_, TK_DIV_, TK_MOD_, // += -= *= /= %=
	TK_LT, TK_LE, TK_GT, TK_GE, TK_EQ, TK_NE,    // < <= > >= == !=
	TK_AND, TK_OR, TK_NOT,                       // && || !
	TK_BIT_AND,	                                 // &
	TK_STO, TK_SEM, TK_COM,                      // = ; ,
	TK_RB_L, TK_RB_R, TK_SB_L, TK_SB_R, TK_CB_L, TK_CB_R, // ( ) [ ] { }
	TK_POUND,									 // #
} tokenType;
extern const char *tokenToStr[];

// node type
enum
{
	/* statement */
	SM_IF, SM_WHILE, SM_RETURN, SM_BREAK, 
	SM_VAR_DEF, SM_FUN_DEF, SM_PARAM_DEF,
	/* exprssion */
	EXP_OP, EXP_CONST, EXP_CONST_F, EXP_STR, EXP_ID, EXP_FUN_CALL,
	EXP_CAST,
	/* no-type */
	NODE_TEMP
};

// cast type
enum { CAST_NONE, CAST_FI, CAST_IF, CAST_P }; // float to integer, integer to float, have pointer

// variable type
enum
{
	VAR_INT, VAR_CHAR, VAR_FLOAT, VAR_POINTER, VAR_ARRAY,
	VAR_OVERLOAD,
	VAR_FUN, VAR_LIBFUN, VAR_ERROR, VAR_VOID
};
extern const char *typeToStr[];
extern const int SIZEOF[];
typedef struct typeInfo
{
	char t;					// type
	struct typeInfo* n;		// next
} typeInfo;

// function scope
#define GLOBAL_SCOPE	0
#define LIBFUN_SCOPE	-1
#define USERFUN_SCOPE	-2

// tree node
#define MAX_CHILDREN (3)
typedef struct treeNode
{
	struct treeNode *child[MAX_CHILDREN];
	struct treeNode *sibling;
	int lineno;
	char type; // node type, statement or exprssion 
	typeInfo dataType;
	union
	{
		int ival;
		float fval;
		char *sval;
	} attr;
} treeNode;

// variable object
typedef struct
{
	union
	{
		int ival;
		float ftval;
		char *sval;
		struct
		{
			treeNode *address;
			int frameSize;
		} fval;
		int *aval; // dims
	} val;
	typeInfo type;
} Object;

// bytecode infomation
typedef struct
{
	int RAMSize;
	int staticSize;
	int globalSize;
	int codeSize;
	char forceBreak;
	char bigFont;
} byteCodeInfo;


#endif