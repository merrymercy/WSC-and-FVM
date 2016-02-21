/****************************************************/
/* File: opt.c                                      */
/* code optimization                                */
/* implementation for the WSC compiler              */
/****************************************************/

#include "globals.h"
#include "parser.h"

// const node type
enum{ CONST_NONE, CONST_LIRI, CONST_LIRF, CONST_LFRI, CONST_LFRF };
// LIRF = left integer, right float

static void freeChild( treeNode *t )
{
	int i;
	
	for( i = 0; i < MAX_CHILDREN; ++i )
	{
		freeTree( t->child[i] );
		t->child[i] = NULL;
	}
}

/* folds constant for a syntax tree */
void foldConstant( treeNode *t )
{
	int i, type;

	if( t == NULL )
		return;

	type = CONST_NONE;
	for( i = 0; i < MAX_CHILDREN; ++i )
		foldConstant( t->child[i] );

	if( t->type == EXP_OP && t->child[0] != NULL && t->child[1] != NULL )
	{
		float left, right, result;
		char isFloat;

		switch( t->child[0]->type )
		{
			case EXP_CONST:
				switch( t->child[1]->type )
				{
					case EXP_CONST:   type = CONST_LIRI; break;
					case EXP_CONST_F: type = CONST_LIRF; break;
					default: goto end;
				}
				break;
			case EXP_CONST_F:
				switch( t->child[1]->type )
				{
					case EXP_CONST:   type = CONST_LFRI; break;
					case EXP_CONST_F: type = CONST_LFRF; break;
					default: goto end;
				}
				break;
			default: goto end;
		}

		switch( type )
		{
			case CONST_LIRI:
				left  = (float)t->child[0]->attr.ival;
				right = (float)t->child[1]->attr.ival;
				isFloat = FALSE;
				break;
			case CONST_LIRF:
				left  = (float)t->child[0]->attr.ival;
				right = (float)t->child[1]->attr.fval;
				isFloat = TRUE;
				break;
			case CONST_LFRI:
				left  = (float)t->child[0]->attr.fval;
				right = (float)t->child[1]->attr.ival;
				isFloat = TRUE;
				break;
			case CONST_LFRF:
				left  = (float)t->child[0]->attr.fval;
				right = (float)t->child[1]->attr.fval;
				isFloat = TRUE;
				break;
			default: break;
		}

		switch( t->attr.ival )
		{
			case TK_ADD: result = left + right; break;
			case TK_SUB: result = left - right; break;
			case TK_MUL: result = left * right; break;
			case TK_DIV: result = left / right; break;
			case TK_MOD: result = (float)((int)left % (int)right); break;
			default: goto end;
		}

		if( isFloat == TRUE )
		{
			t->type = EXP_CONST_F;
			t->attr.fval = result;
		}
		else
		{
			t->type = EXP_CONST;
			t->attr.ival = (int)result;
		}

		freeChild( t );
	}

end:

	foldConstant( t->sibling );
}
