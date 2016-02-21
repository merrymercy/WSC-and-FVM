/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the WSC compiler      */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

/* TABSIZE is the size of the hash table */
#define TABSIZE (211)
/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT (4)

/* The record in the bucket lists for
 * each variable
 */
typedef struct Bucket
{
	char *name;
	struct Bucket *next;
	Object obj;
	int scope;
	int loc; /* memory location for variable, or entry for
				function */
} Bucket;

/* Function SymtabLookup returns the Bucket 
 * pointer to the symbol
 */
Bucket *symtabLookup( const char *name, int scope );
Bucket *searchDouble( const char *name, int first, int second );

/* Procedure SymtabInsert inserts scope and
 * memory locations into the symbol table
 */
void symtabInsert(  const char *name, Object *obj, int scope, int loc,
				  int lineno );

/* free the symbol table */
void freeSymtab( void );

#endif
