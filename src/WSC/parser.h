/****************************************************/
/* File: parser.h                                   */
/* The parser interface for the WSC compiler        */
/****************************************************/

#ifndef _PARSER_H_
#define _PARSER_H_

/* return the newly constructed syntax tree */
treeNode *parse( char *src );

/* creates a new type information for treeNode */
typeInfo *newTypeInfo( char kind );

/* creates a new tree node for syntax tree */
treeNode *newNode( char kind );

/* allocates and makes a new copy of an existing string */
char *copyString( const char *str );

/* free a syntax tree */
void freeTree( treeNode *node );

/* free a typeInfo struct */
void freeTypeInfo( void );

#endif

