/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the WSC compiler */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/****************************************************/

#include "globals.h"
#include "parser.h"
#include "symtab.h"

Bucket *symbolTable[TABSIZE];

/* the hash function */
static int hash( const char *key, int scope )
{
	int i, temp;

	temp = scope + 100;
	for( i = 0; key[i] != '\0'; ++ i )
		temp = ((temp << SHIFT) + key[i] ) % TABSIZE;

	return temp;
}

/* returns the Bucket pointer to the symbol */
Bucket *symtabLookup( const char *name, int scope )
{
	int index = hash( name, scope );
	Bucket *bp = symbolTable[index];

	while( bp != NULL && ( strcmp( bp->name, name ) || bp->scope
		!= scope) )
		bp = bp->next;

	return bp;
}

/* search in local and global scope */
Bucket *searchDouble( const char *name, int first, int second )
{
	Bucket *bp;

	bp = symtabLookup( name, first );
	if( bp == NULL )
		bp = symtabLookup( name, second );

	return bp;
}

/* inserts scope and memory locations into the symbol table */
void symtabInsert(  const char *name, Object *obj, int scope, int loc,
				  int lineno )
{
	int index = hash( name, scope );

	Bucket *head = symbolTable[index];
	Bucket *find = symtabLookup( name, scope );

	if( find == NULL )
	{
		Bucket *bp = (Bucket *)malloc( sizeof(Bucket) );
		if( bp == NULL )
			outOfMem( lineno, "bucket" );
		bp->name = copyString( name );
		memcpy( &bp->obj, obj, sizeof(Object) );
		bp->scope = scope;
		bp->loc = loc;
		bp->next = head;
		symbolTable[index] = bp;
	}
}

/* free the symbol table */
void freeSymtab( void )
{
	int i;

	for( i = 0; i < TABSIZE; ++i )
	{
		Bucket *bp = symbolTable[i];
		Bucket *temp;
		while( bp != NULL )
		{
			free( bp->name );
			if( bp->obj.type.t == VAR_ARRAY && bp->obj.val.aval != NULL )
				free( bp->obj.val.aval );
			temp = bp->next;
			free( bp );
			bp = temp;
		}
	}
}