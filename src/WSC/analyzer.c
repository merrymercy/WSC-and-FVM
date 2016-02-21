/****************************************************/
/* File: analyzer.c                                 */
/* Semantic analyzer implementation                 */
/* for the WSC compiler                             */
/****************************************************/

#include "globals.h"
#include "parser.h"
#include "analyzer.h"
#include "symtab.h"
#include "lib.h"

/* counter for variable memory locations and scope */
static int memloc;
static int scope;

char *funName;

const int SIZEOF[] =
{
	sizeof(int), sizeof(char), sizeof(float), sizeof(int*), sizeof(int*),
	0, 0, 0, 0, 0,
};

static void addMemloc( char type, int size )
{
	memloc += SIZEOF[type] * size;
}

/* null is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void null( treeNode *node )
{
	return;
}

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( treeNode *t, void (*preProc)(treeNode *),
					 void (*postProc)(treeNode *) )
{
	if( t != NULL )
	{
		int i;
		preProc(t);
		for( i = 0; i < MAX_CHILDREN; ++i )
            traverse( t->child[i], preProc, postProc );
		postProc(t);
		traverse( t->sibling, preProc, postProc );
	}
}

static void varDef( treeNode *node )
{
	Object obj;
	if( node->dataType.t == VAR_ARRAY ) //array
	{
		if( symtabLookup( node->attr.sval, scope ) == NULL )
		{
			int size = 1;
			int *aval = malloc( 2*sizeof(int) );
			int dims = 1;
			char baseType = VAR_CHAR;
			treeNode *t = node->child[0];
			typeInfo *type = node->dataType.n;

			do
			{
				if( t->type != EXP_CONST )
					throwError( node->lineno, "Array size must be a const" );
				if( t->attr.ival <= 0 )
					throwError( node->lineno, "Array size must be a positive number" );

				aval = realloc( aval, (dims+2)*sizeof(int) );
				aval[dims+1] = t->attr.ival;
				size *= t->attr.ival;
				dims++;

				t = t->sibling;
			} while( t != NULL );

			do
			{
				if( type->t != VAR_ARRAY )
					break;

				type = type->n;
			} while( 1 );
			baseType = type->t;

			memcpy( &obj.type, &node->dataType, sizeof( typeInfo ) );
			aval[0] = obj.val.ival = size;
			aval[1] = dims - 1;
			obj.val.aval = aval;
			symtabInsert( node->attr.sval, &obj, scope, memloc, node->lineno );
			addMemloc( baseType, size );
		}
		else
			throwError( node->lineno, "Variable '%s' redefinition",
					node->attr.sval );
	}
	else
	{
		if( symtabLookup( node->attr.sval, scope ) == NULL )
		{
			obj.val.ival = 0;
			memcpy( &obj.type, &node->dataType, sizeof( typeInfo ) );

			symtabInsert( node->attr.sval, &obj, scope, memloc, node->lineno );
			addMemloc( node->dataType.t, 1 );
		}
		else
			throwError( node->lineno, "Variable '%s' redefinition",
					node->attr.sval );
	}
}


static unsigned char *data; // data buffer
static int dataLoc = 0;
/* insert const string into the symbol table */
static void insertString( treeNode *node )
{
	if( node->type == EXP_STR )
	{
		char *s = node->attr.sval;
		char ch = *s;

		// avoid use the same name as global variable
		// because they are in the same scope -- 0
		// variable can't begin with digit, so i change
		// it to '0'
		if( ch )
			*s = '0';

		if( symtabLookup( s, GLOBAL_SCOPE ) == NULL )
		{
			Object obj;
			int len;

			/* add to symbol table */
			obj.val.sval = node->attr.sval;
			obj.type.t = VAR_ARRAY;
			obj.type.n = newTypeInfo( VAR_CHAR );
			obj.val.aval = NULL;
			symtabInsert( s, &obj, 0, memloc, node->lineno );
			len = strlen( s ) + 1;
			addMemloc( VAR_CHAR, len );

			/* add to data buffer */
			*s = ch;
			data = realloc( data, dataLoc + len );
			strcpy( data + dataLoc, s );
			dataLoc += len;
			if( ch )
				*s = '0';
		}
		/* change type */
		node->type = EXP_ID;
	}
}

/* insert global variable into the symbol table */
static void insertGlobal( treeNode *root )
{
	treeNode *node;

	for( node = root; node != NULL; node = node->sibling )
	{
		if( node->type == SM_VAR_DEF )
			varDef( node );
		else if( node->type == SM_FUN_DEF )
		{
			Object obj;

			if( searchDouble( node->attr.sval, LIBFUN_SCOPE, USERFUN_SCOPE )
				== NULL )
			{
				obj.val.fval.address = node;
				memcpy( &obj.type, &node->dataType, sizeof( typeInfo ) );
				symtabInsert( node->attr.sval, &obj, USERFUN_SCOPE, 0, node->lineno );
			}
			else
			{
				if( symtabLookup( node->attr.sval, LIBFUN_SCOPE ) )
					// is already lib function
					throwError( node->lineno, "Function '%s' redefinition. "
						"It is a lib function.",	node->attr.sval );
				else
					throwError( node->lineno, "Function '%s' redefinition.",
						node->attr.sval );
			}
		}
		else
			throwError( node->lineno, "Illegal statement in global scope" );
	}

	if( symtabLookup( "main", USERFUN_SCOPE ) == NULL )
		throwError( -1, "Undeclared function: 'main'" );
}

/* insert local variable into the symbol table */
static void insertLocal( treeNode *node )
{
	switch( node->type )
	{
		case SM_VAR_DEF:
		case SM_PARAM_DEF:
			varDef( node );
			break;
		case EXP_ID:
			if( searchDouble( node->attr.sval, scope, GLOBAL_SCOPE ) == NULL )
				throwError( node->lineno, "Undeclared identifier: '%s'",
					node->attr.sval );
			break;
		case EXP_FUN_CALL:
			if( searchDouble( node->attr.sval, LIBFUN_SCOPE, USERFUN_SCOPE ) == NULL )
				throwError( node->lineno, "Undeclared function: '%s'",
					node->attr.sval );
			break;
		default : break;
	}
}

/* constructs the symbol table by preorder traversal of the syntax tree
 * and returns the static size and global size of program
 */
unsigned char *buildSymtab( treeNode *root, byteCodeInfo *info )
{
	treeNode *node;

	memloc = 0;

	traverse( root, insertString, null );
	info->staticSize = memloc;

	insertGlobal( root );
	info->globalSize = memloc;

	if( errorOccurred == TRUE )
		return NULL;

	scope = 0;
	for( node = root; node != NULL; node = node->sibling )
	{
		if( node->type == SM_FUN_DEF )
		{
			memloc = 0;
			++scope;
			traverse( node->child[0], insertLocal, null );
			traverse( node->child[1], insertLocal, null );
			symtabLookup( node->attr.sval, USERFUN_SCOPE )->obj.val.fval.frameSize = memloc;
		}
	}

	return info->staticSize == 0 ? NULL : data;
}

/* compare t1 and t2, return the higher type */
static typeInfo *typePromotion( typeInfo *t1, typeInfo *t2, char cast[2] )
{
	if( t1 == NULL || t2 == NULL )
		return 0;

	switch( t1->t )
	{
		case VAR_FLOAT:
			switch( t2->t )
			{
				case VAR_INT: case VAR_CHAR:
					cast[0] = CAST_NONE; cast[1] = CAST_IF;
					return t1;
				case VAR_FLOAT:
					cast[0] = cast[1] = CAST_NONE;
					return t1;
			}
			break;
		case VAR_INT: case VAR_CHAR:
			switch( t2->t )
			{
				case VAR_INT: case VAR_CHAR:
					cast[0] = cast[1] = CAST_NONE;
					return t1;
				case VAR_POINTER: case VAR_ARRAY:
					cast[0] = cast[1] = CAST_P;
					return t1;
				case VAR_FLOAT:
					cast[0] = CAST_IF; cast[1] = CAST_NONE;
					return t2;
			}
			break;
		case VAR_POINTER: case VAR_ARRAY:
			switch( t2->t )
			{
				case VAR_INT: case VAR_CHAR:
					cast[0] = cast[1] = CAST_NONE;
					return t1;
				case VAR_POINTER: case VAR_ARRAY:
					{
						typeInfo *r = t1;
						//r = typePromotion( t1->n, t2->n, cast );
						cast[0] = cast[1] = CAST_NONE;
						return r;
					}
			}
		case VAR_VOID:
			if( t2->t == VAR_VOID )
			{
				cast[0] = cast[1] = CAST_NONE;
				return t1;
			}
			break;
	}

	cast[0] = cast[1] = CAST_NONE;
	return NULL;
}

/* add Cast for integer and float mixed mode operation, like 5 * 6.5 */
static void addCast( char cast, treeNode **node )
{
	treeNode *t;
	tokenType token = (*node)->attr.ival;

	if( cast == CAST_P ) // cast_p is for overload lib function
		return;

	t = newNode( EXP_CAST );
	t->attr.ival = cast;
	t->child[0] = *node;
	t->lineno = (*node)->lineno;
	t->dataType.n = NULL;
	switch( cast )
	{
		case CAST_FI:
			t->dataType.t = VAR_INT;
			break;
		case CAST_IF:
			t->dataType.t = VAR_FLOAT;
			break;
		default:
			t->dataType.t = VAR_ERROR;
			break;
	}

	*node = t;
}

/* there is so difficult, or I do it in a bad way */
static void checkNode( treeNode *node );
static treeNode *translateIndex( treeNode *index, int *dimInfo, char *isPointer )
{
	treeNode *left, *result, *next, **last, **now;
	int i, offset, product;
	int upper = dimInfo[0] + 1; /* dimInfo[0] is the upper bound of the array */

	now = &result;
	offset = 2;

	while( index != NULL )
	{
		next = index->sibling;   // save sibling, because it will be changed below

		left = newNode( EXP_OP );
		left->attr.ival = TK_MUL;

		left->child[0] = index;
		left->child[0]->sibling = NULL; // it's the reason to save sibling
		left->child[1] = newNode( EXP_CONST );

		/* compute size */
		product = 1;
		for( i = offset; i < upper; ++i )
			product *= dimInfo[i];
		++offset;

		left->child[1]->attr.ival = product;

		*now = newNode( EXP_OP );
		(*now)->attr.ival = TK_ADD;
		(*now)->child[0] = left;

		last = now;
		now = &((*now)->child[1]);
		index = next;
	}

	*last = left;

	if( offset <= upper ) // is a pointer
		*isPointer = TRUE;
	else
		*isPointer = FALSE;

	// patch dataType for the treeNodes that created just now
	traverse( result, null, checkNode );

	return result;
}

static void checkNode( treeNode *node )
{
	switch( node->type )
	{
		case SM_IF:
		case SM_WHILE:
			{
				typeInfo type = { VAR_INT, NULL };
				char cast[2];

				if( typePromotion( &type, &node->child[0]->dataType, cast) == NULL )
					throwError( node->lineno, "Illegal test statement, type ="
						" '%s'", typeToStr[0] );
				if( node->child[0]->dataType.t == VAR_FLOAT )
					addCast( CAST_FI, &node->child[0] );
			}
			break;
		case SM_RETURN:
			{
				typeInfo retType, *funType;
				char cast[2];

				if( node->child[0] == NULL )
				{
					retType.t = VAR_VOID; retType.n = NULL;
				}
				else
					memcpy( &retType, &node->child[0]->dataType, sizeof( typeInfo ) );

				funType = symtabLookup( funName, USERFUN_SCOPE )->obj.type.n;

				if( typePromotion( funType, &retType, cast ) == NULL )
					throwError( node->lineno, "Type error, return '%s' to "
						"'%s'", typeToStr[retType.t], typeToStr[funType->t] );

				if( cast[0] == CAST_IF )
					addCast( CAST_FI, &node->child[0] );
				else if( cast[1] != CAST_NONE )
					addCast( cast[1], &node->child[0] );
			}
			break;
		case SM_VAR_DEF:
			{
				int i;
				treeNode *t = node->child[MAX_CHILDREN-1];

				if( node->dataType.t != VAR_ARRAY )
					break;

				for( i = 0; t != NULL; t = t->sibling, ++i )
					;

				if( i > *(symtabLookup( node->attr.sval, scope )->obj.val.aval) )
					throwError( node->lineno, "too many initializers" );
			}
			break;
		case EXP_OP:
			{
				typeInfo type = { VAR_INT, NULL };
				typeInfo *result = &type;
				char cast[2];

				if( node->child[0] == NULL )
				{
					tokenType op = node->attr.ival;

					if( op == TK_NOT )
					{
						result = typePromotion( &type, &node->child[1]->dataType, cast);
						if( cast[0] == CAST_IF )
							addCast( cast[0], &node->child[1] );
					}
					else if( op == TK_BIT_AND )
					{
						treeNode *t = node->child[1];

						if( t->type != EXP_ID )
							throwError( node->lineno, "'&' on constant" );
					}
					else if( op == TK_MUL )
					{
						treeNode *t = node->child[1];
						char ok = TRUE;
						Bucket *bp;

						if( t->type != EXP_ID )
							ok = FALSE;
						else
						{
							bp = searchDouble( t->attr.sval, scope, GLOBAL_SCOPE );
							if( bp == NULL || bp->obj.type.t != VAR_POINTER )
								ok = FALSE;
						}

						if( !ok )
							throwError( node->lineno, "Illegal indirection" );
						else
							memcpy( &type, bp->obj.type.n, sizeof(typeInfo) );
					}
					cast[0] = cast[1] = CAST_NONE;
				}
				else
					result = typePromotion( &node->child[0]->dataType,
										&node->child[1]->dataType, cast );

				if( result == NULL )
					throwError( node->lineno, "Type error, '%s %s %s'",
						node->child[0]==NULL?"":typeToStr[node->child[0]->dataType.t],
						tokenToStr[node->attr.ival],
						node->child[1]==NULL?"":typeToStr[node->child[1]->dataType.t] );
				else if( node->attr.ival == TK_STO )
				{
					treeNode *t = node->child[0];
					char ok = FALSE;

					if( t->type == EXP_ID && t->dataType.t != VAR_ARRAY )
						ok = TRUE;
					else if( t->type == EXP_OP && t->attr.ival == TK_MUL &&
						t->child[0] == NULL ) // dereference
						ok = TRUE;

					if( !ok )
						throwError( node->lineno, "left operand must be l-value" );

					if( cast[0] == CAST_IF )
						addCast( CAST_FI, &node->child[1] );
					else if( cast[1] != CAST_NONE )
						addCast( cast[1], &node->child[1] );

					memcpy( &node->dataType, result, sizeof(typeInfo) );
				}
				else
				{
					tokenType token = node->attr.ival;

					if( cast[0] != CAST_NONE )
						addCast( cast[0], &node->child[0] );
					if( cast[1] != CAST_NONE )
						addCast( cast[1], &node->child[1] );

					if( token == TK_LT || token == TK_LE || token == TK_GT 
						|| token == TK_GE || token == TK_EQ || token == TK_NE
						|| token == TK_AND || token == TK_OR || token == TK_NOT )
					{
						node->dataType.t = VAR_INT;
					}
					else
						memcpy( &node->dataType, result, sizeof(typeInfo) );

					if( node->attr.ival == TK_MOD && node->dataType.t == VAR_FLOAT )
						throwError( node->lineno, "'%%' must on integer" );
				}
			}
			break;
		case EXP_CONST:
			node->dataType.t = VAR_INT;
			break;
		case EXP_CONST_F:
			node->dataType.t = VAR_FLOAT;
			break;
		case EXP_ID:
			{
				Bucket *bp = searchDouble( node->attr.sval, scope, GLOBAL_SCOPE );

				if( bp->obj.type.t == VAR_POINTER || bp->obj.type.t == VAR_ARRAY )
				{
					if( node->child[0] == NULL )
						memcpy( &node->dataType, &bp->obj.type, sizeof(typeInfo) );
					else
					{
						treeNode *t = node->child[0];
						typeInfo *type = &bp->obj.type;
						int dim = 0;

						do
						{
							t = t->sibling;
							type = type->n;
							++dim;
						} while( t != NULL && type->n != NULL );

						if( bp->obj.type.t == VAR_ARRAY ) // pointers don't have aval
						{
							char isPointer;

							if( dim == bp->obj.val.aval[1] && t != NULL )
								throwError( node->lineno, "Out of dims" );

							// translate multidimensional index to one-dimensional index
							if( bp->obj.val.aval[1] > 1 )
								node->child[0] = translateIndex( node->child[0],
								bp->obj.val.aval + 1, &isPointer );

							// is a pointer, so add '&' operation
							if( isPointer == TRUE )
							{
								treeNode *t;

								t = newNode( EXP_OP );
								t->attr.ival = TK_BIT_AND;
								t->child[1] = newNode( NODE_TEMP );

								// warning!!!!!!!!!!!!!!
								t->sibling = node->sibling;
								node->sibling = NULL;

								//memcpy( &node->dataType, type, sizeof(typeInfo) );

								memcpy( t->child[1], node, sizeof(treeNode) );
								memcpy( node, t, sizeof(treeNode) );
							}
						}

						memcpy( &node->dataType, type, sizeof(typeInfo) );
					}
				}
				else
				{
					if( node->child[0] == NULL )
						node->dataType.t = bp->obj.type.t;
					else
						throwError( node->lineno, "'%s' is not array", bp->name );
				}
			}
			break;
		case EXP_FUN_CALL:
			{
				Bucket *bp = searchDouble( node->attr.sval, LIBFUN_SCOPE, USERFUN_SCOPE );
				treeNode **n1 = &node->child[0]; // pointer to pointer, for calling addCast()
				char cast[2];

				if( bp->scope == USERFUN_SCOPE ) /* user's function */
				{
					treeNode *n2 = bp->obj.val.fval.address->child[0];

					while( *n1 != NULL && n2 != NULL )
					{
						treeNode *t = (*n1)->sibling;

						if( typePromotion( &n2->dataType, &(*n1)->dataType,
							cast ) == NULL )
							throwError( node->lineno, "Type error in call '%s()'"
								", translate '%s' to '%s'", node->attr.sval,
								typeToStr[(*n1)->dataType.t], typeToStr[n2->dataType.t]);

						if( cast[0] == CAST_IF )
						{
							addCast( CAST_FI, n1 );
							(*n1)->child[0]->sibling = NULL;
						}
						// don't use "cast[1] != CAST_DONE" !!, because when cast[1] == CAST_P,
						// (*n1)->child[0] is NULL
						else if( cast[1] == CAST_FI || cast[1] == CAST_IF )
						{
							addCast( cast[1], n1 );
							(*n1)->child[0]->sibling = NULL;
						}

						(*n1)->sibling = t;
						n1 = &(*n1)->sibling; n2 = n2->sibling;
					}

					if( *n1 != NULL || n2 != NULL )
						throwError( node->lineno, "Wrong number of %s()'s parameters",
							node->attr.sval );
				}
				else /* lib's function */
				{
					libFun *fun = (libFun *)bp->obj.val.fval.address;
					typeInfo n2;
					char *args = fun->args;

					if( fun->retType != VAR_OVERLOAD )
					{
						n2.t = *args++;
						while( *n1 != NULL && n2.t != VAR_VOID )
						{
							treeNode *t = (*n1)->sibling;

							if( typePromotion( &n2, &(*n1)->dataType,cast ) == NULL )
								throwError( node->lineno, "Type error in call '%s()'"
									", translate '%s' to '%s'", node->attr.sval,
									typeToStr[(*n1)->dataType.t], typeToStr[n2.t]);

							if( cast[0] == CAST_IF )
							{
								addCast( CAST_FI, n1 );
								(*n1)->child[0]->sibling = NULL;
							}
							// don't use "cast[1] != CAST_DONE" !!, because when cast[1] == CAST_P,
							// (*n1)->child[0] is NULL
							else if( cast[1] == CAST_FI || cast[1] == CAST_IF )
							{
								addCast( cast[1], n1 );
								(*n1)->child[0]->sibling = NULL;
							}

							(*n1)->sibling = t;
							n1 = &(*n1)->sibling; n2.t = *args++;
						}

						if( *n1 != NULL || n2.t != VAR_VOID )
							throwError( node->lineno, "Wrong number of %s()'s parameters",
								node->attr.sval );
					}
					else
					{
						treeNode *n1;
						int i;
						char matched;

						for( i = 1; i <= fun->vector; ++i )
						{
							matched = TRUE;
							n1 = node->child[0];
							args = ((libFun *)fun+i)->args;
							n2.t = *args++;

							while( n1 != NULL && n2.t != VAR_VOID && matched == TRUE )
							{
								if( typePromotion( &n2, &n1->dataType,cast ) == NULL )
									matched = FALSE;

								if( cast[0] != CAST_NONE || cast[1] != CAST_NONE )
									matched = FALSE;

								n1 = n1->sibling; n2.t = *args++;
							}

							if( n1 != NULL || n2.t != VAR_VOID )
								matched = FALSE;

							if( matched == TRUE )
								break;
						}

						if( i > fun->vector )
							throwError( node->lineno, "Error in call '%s()'", node->attr.sval );
						else
						{
							free( node->attr.sval );
							node->attr.sval = copyString( ((libFun *)fun+i)->name );
						}
					}
				}

				node->dataType.t = bp->obj.type.n->t;
			}
			break;
		default: break;
	}
}

/* performs type checking by a postorder syntax tree traversal */
void typeCheck( treeNode *root )
{
	treeNode *node;

	memloc = 0;
	scope = 0;
	for( node = root; node != NULL; node = node->sibling )
	{
		if( node->type == SM_FUN_DEF )
		{
			memloc = 0;
			++scope;
			funName = node->attr.sval;
			traverse( node->child[1], null, checkNode );
		}
		else if( node->type == SM_VAR_DEF )
		{
			if( node->child[MAX_CHILDREN-1] != NULL )
				traverse( node->child[MAX_CHILDREN-1], null, checkNode );
		}
	}
}
