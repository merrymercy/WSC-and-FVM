/****************************************************/
/* File: parser.c                                   */
/* The parser implementation for the WSC compiler   */
/****************************************************/

#include "globals.h"
#include "scanner.h"
#include "prepro.h"
#include "parser.h"

/* holds current token */
static tokenType token;

static char unmatched = FALSE;
static int inWhile = 0;
static int dimsTotal;
static int dimsStack[10];
static int dimsTop = 0;

/* function prototypes for recursive calls */
static treeNode *declaration_list();
static treeNode *declaration();
static treeNode *var_declaration( typeInfo *type, char *name );
static treeNode *fun_declaration( typeInfo *type, char *name );
static treeNode *param_list();
static treeNode *param();
static treeNode *compound_stmt();
static treeNode *statement_list();
static treeNode *statement();
static treeNode *expression_stmt();
static treeNode *if_stmt();
static treeNode *while_stmt();
static treeNode *for_stmt();
static treeNode *return_stmt();
static treeNode *break_stmt();
static treeNode *expression();
static treeNode *bool_or_exp();
static treeNode *bool_and_exp();
static treeNode *bool_equ_exp();
static treeNode *bool_rel_exp();
static treeNode *simple_exp();
static treeNode *term();
static treeNode *unary();
static treeNode *factor();
static treeNode *var();
static treeNode *args();

/* for free */
#define MAX_VALTYPE 200
static typeInfo *typeinfoP[MAX_VALTYPE];
static int top = 0;

/* creates a new type information for treeNode */
typeInfo *newTypeInfo( char kind )
{
	typeInfo *t = (typeInfo *)malloc( sizeof(typeInfo) );

	if( t == NULL || top >= MAX_VALTYPE )
		outOfMem( lineno, "type infomation" );
	else
	{
		typeinfoP[top++] = t;
		t->t = kind;
		t->n = NULL;
	}

	return t;
}
/* creates a new tree node for syntax tree */
treeNode *newNode( char kind )
{
	treeNode *t = (treeNode *)malloc( sizeof(treeNode) );

	if( t == NULL )
		outOfMem( lineno, "tree node" );
	else
	{
		memset( t->child, 0, sizeof(t->child[0]) * MAX_CHILDREN );
		t->sibling = NULL;
		t->lineno = lineno;
		t->type = kind;
		t->dataType.t = VAR_ERROR;
		t->dataType.n = NULL;
	}
	return t;
}
/* allocates and makes a new copy of an existing string */
char *copyString( const char *str )
{
	char *t;

	t = malloc( strlen( str ) + 1 );
	if( t == NULL )
		outOfMem( lineno, "copy sring" );
	else
		strcpy( t, str );
	return t;
}

static void match( tokenType expected )
{
	if( token == expected )
		token = getToken();
	else
	{
		switch( expected )
		{
			case TK_ID:
				throwError( lineno, "Expect a identifier before '%s'",
					tokenToStr[token] );
				break;
			default:
				throwError( lineno, "Missing '%s' before %s",
					tokenToStr[expected], tokenToStr[token] );
				break;
		}
		unmatched = TRUE;
	}
}
/****************************************/
/* the primary function of the parser   */
/****************************************/
/* returns the newly constructed syntax tree */
treeNode *parse( char *src )
{
	setScanner( src, 1, 0 );

	token = getToken();
	return declaration_list();
}
static treeNode *declaration_list()
{
	treeNode *root, *prev;
	root = NULL;

	while( token != TK_EOF && token != TK_ERROR )
	{
		treeNode *node = declaration();

		if( node != NULL )
		{
			if( root == NULL )
				root = prev = node;
			else
			{
				while( prev->sibling != NULL )
					prev = prev->sibling;
				prev->sibling = node;
				prev = node;
			}
		}
	}

	return root;
}
static typeInfo type_specifier( char putError )
{
	typeInfo type = { VAR_ERROR, NULL };

	switch( token )
	{
		case TK_VOID : type.t = VAR_VOID; break;
		case TK_INT : type.t = VAR_INT; break;
		case TK_CHAR : type.t = VAR_CHAR; break;
		case TK_FLOAT : type.t = VAR_FLOAT; break;
		default :
			if( putError )
				throwError( lineno, "Invalid type(expect a type specifier) ->"
				" '%s'", tokenToStr[token] );
			return type;
	}
	match( token );

	if( token == TK_MUL ) // pointer
	{
		match( token );
		type.n = newTypeInfo( type.t );
		type.t = VAR_POINTER;
	}

	return type;
}
static treeNode *declaration()
{
	typeInfo dataType;
	char *name;

	dataType = type_specifier( TRUE );
	name = copyString( tokenString );
	match( TK_ID );

	if( dataType.t == VAR_ERROR )
		match( token );

	if( token == TK_SEM || token == TK_COM || token == TK_SB_L
		|| token == TK_STO )
		return var_declaration( &dataType, name );
	else if( token == TK_RB_L )
		return fun_declaration( &dataType, name );
	else
		throwError( lineno, "Missing ';' after declaration" );

	return NULL;
}
static treeNode *var_init( treeNode *left, char isArray )
{
	treeNode *t = newNode( EXP_OP );

	t->attr.ival = TK_STO;

	t->child[0] = newNode( EXP_ID );
	t->child[0]->attr.sval = copyString( left->attr.sval );

	if( isArray == TRUE ) // for array
	{
		treeNode **node = &t->child[0]->child[0];
		int i;

		for( i = 0; i < dimsTop; ++i, node = &(*node)->sibling )
		{
			*node = newNode( EXP_CONST );
			(*node)->attr.ival = dimsStack[i];
		}
	}

	t->child[1] = expression();

	return t;
}
static treeNode *array_init( treeNode *left, int i )
{
	treeNode *root, *prev;
	int index = 0;

	root = NULL;

	match( TK_CB_L );
	dimsTop++; // push 

	while( token != TK_CB_R )
	{
		treeNode *node;
		dimsStack[dimsTop-1] = index++;
		
		if( i == dimsTotal - 1 )
			node = var_init( left, TRUE );
		else
			node = array_init( left, i+1 );

		if( node != NULL )
		{
			if( root == NULL )
				root = prev = node;
			else
			{
				while( prev->sibling != NULL )
					prev = prev->sibling;
				prev->sibling = node;
				prev = node;
			}
		}
		if( token != TK_CB_R )
			match( TK_COM );
		if( unmatched )
		{
			unmatched = FALSE;
			break;
		}
	}

	/* pop */
	dimsStack[--dimsTop];
	match( TK_CB_R );

	return root;
}
static treeNode *var_declaration( typeInfo *type, char *name )
{
	treeNode *t = newNode( SM_VAR_DEF );

	if( type->t == VAR_VOID )
		throwError( lineno, "Invalid type -> 'void'" );

	t->attr.sval = name;

	if( token == TK_SB_L ) //array
	{
		treeNode **node = &t->child[0];
		typeInfo *now = &t->dataType;

		dimsTotal = 0;
		do
		{
			match( TK_SB_L );
			now->t = VAR_ARRAY;
			now->n = newTypeInfo( VAR_ERROR );
			*node = expression();
			match( TK_SB_R );

			now = now->n;
			node = &(*node)->sibling;
			++dimsTotal;
		}while( token == TK_SB_L );

		memcpy( now, type, sizeof(typeInfo) );
	}
	else
		memcpy( &t->dataType, type, sizeof(typeInfo) );

	if( token == TK_STO ) // like int a = 5;
	{
		match( token );
		if( token == TK_CB_L ) // array
		{
			t->child[MAX_CHILDREN-1] = array_init( t, 0 );
		}
		else
			t->child[MAX_CHILDREN-1] = var_init( t, FALSE );
// warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}

	if( token == TK_COM ) // like int a,b,c;
	{
		char *name;

		match( token );
		name = copyString( tokenString );
		match( TK_ID );

		if( token == TK_SEM || token == TK_COM || token == TK_SB_L
			|| token == TK_STO )
			t->sibling = var_declaration( type, name );
	}
	else
		match( TK_SEM );

	return t;
}
static treeNode *fun_declaration( typeInfo *type, char *name )
{
	treeNode *t = newNode( SM_FUN_DEF );

	t->attr.sval = name;
	t->dataType.t = VAR_FUN;
	t->dataType.n = newTypeInfo( VAR_ERROR );
	memcpy( t->dataType.n, type, sizeof(typeInfo) );

	match( TK_RB_L );
	t->child[0] = param_list();
	match( TK_RB_R );
	t->child[1] = compound_stmt();

	return t;
}
static treeNode *param_list()
{
	treeNode *root, *prev;
	root = NULL;

	if( token == TK_VOID )
		match( token );

	while( token != TK_RB_R )
	{
		treeNode *node = param();

		if( node != NULL )
		{
			if( root == NULL )
				root = prev = node;
			else
			{
				prev->sibling = node;
				prev = node;
			}
		}

		if( token != TK_RB_R )
			match( TK_COM );
		if( unmatched )
		{
			unmatched = FALSE;
			break;
		}
	}

	return root;
}
static treeNode *param()
{
	treeNode *t = newNode( SM_PARAM_DEF );
	typeInfo type;

	type = type_specifier( TRUE );
	memcpy( &t->dataType, &type, sizeof(typeInfo) );

	t->attr.sval = copyString( tokenString );
	match( TK_ID );

	return t;
}
static treeNode *compound_stmt()
{
	treeNode *t;

	match( TK_CB_L );
	t = statement_list();
	match( TK_CB_R );

	return t;
}
static treeNode *statement_list()
{
	treeNode *root, *prev;
	root = NULL;

	while( token != TK_CB_R && token != TK_EOF && token != TK_ERROR )
	{
		treeNode *node = statement();

		if( node != NULL )
		{
			if( root == NULL )
				root = prev = node;
			else
			{
				while( prev->sibling != NULL )
					prev = prev->sibling;
				prev->sibling = node;
				prev = node;
			}
		}
	}

	return root;
}
static treeNode *statement()
{
	switch( token )
	{
		case TK_CB_L : return compound_stmt();
		case TK_IF : return if_stmt();
		case TK_WHILE : return while_stmt();
		case TK_FOR : return for_stmt();
		case TK_RETURN : return return_stmt();
		case TK_BREAK : return break_stmt();
		default :
			{
				typeInfo type = type_specifier( FALSE );
				if( type.t != VAR_ERROR )
				{
					char *name;

					name = copyString( tokenString );
					match( TK_ID );

					return var_declaration( &type, name );
				}
				else
					return expression_stmt();
			}
	}
	return NULL;
}
static treeNode *expression_stmt()
{
	treeNode *t;

	if( token != TK_SEM )
		t = expression();
	else
		t = NULL;

	match( TK_SEM );
	return t;
}
static treeNode *if_stmt()
{
	treeNode *t = newNode( SM_IF );

	match( TK_IF );
	match( TK_RB_L );
	t->child[0] = expression();
	match( TK_RB_R );

	t->child[1] = statement();

	if( token == TK_ELSE )
	{
		match( token );
		t->child[2] = statement();
	}

	return t;
}
static treeNode *while_stmt()
{
	treeNode *t = newNode( SM_WHILE );

	match( TK_WHILE );
	match( TK_RB_L );
	t->child[0] = expression();
	match( TK_RB_R );

	++inWhile;
	t->child[1] = statement();
	--inWhile;

	return t;
}
static treeNode *for_stmt()
{
	treeNode *t, *end, *n;

	match( TK_FOR );
	match( TK_RB_L );
	t = expression();
	match( TK_SEM );
	t->sibling = newNode( SM_WHILE ); /* translate to while */
	t->sibling->child[0] = expression();
	match( TK_SEM );
	end = expression();
	match( TK_RB_R );

	++inWhile;
	n = t->sibling->child[1] = statement();
	--inWhile;

	if( n != NULL )
	{
		while( n->sibling != NULL )
			n = n->sibling;  /* link the end to the while body */
		n->sibling = end;
	}
	else
		t->sibling->child[1] = end;

	return t;
}
static treeNode *return_stmt()
{
	treeNode *t = newNode( SM_RETURN );

	match( TK_RETURN );

	t->child[0] = expression_stmt();

	return t;
}
static treeNode *break_stmt()
{
	match( TK_BREAK );
	if( inWhile )
		return newNode( SM_BREAK );
	else
	{
		throwError( lineno, "illegal break(must in a loop)" );
		return NULL;
	}
}

static treeNode *expression()
{
	treeNode *left = bool_or_exp();

	switch( token )
	{
		case TK_STO:
			{
				treeNode *node = newNode( EXP_OP );
				match( token );

				node->attr.ival = TK_STO;
				node->child[0] = left;
				node->child[1] = expression();
				left = node;
			}
			break;
		case TK_ADD_:case TK_SUB_:case TK_MUL_:case TK_DIV_:case TK_MOD_:
			{
				treeNode *node = newNode( EXP_OP );
				treeNode *t = newNode( EXP_OP );

				node->attr.ival = TK_STO;
				node->child[0] = left;
				node->child[1] = t;

				t->attr.ival = token - 5;
				t->child[0] = newNode( EXP_ID );
				memcpy( t->child[0], left, sizeof(treeNode) );
				t->child[0]->attr.sval = copyString( left->attr.sval );
/* warning !!!!!!!!!! */

				match( token );
				t->child[1] = expression();
				left = node;
			}
			break;
		default: break;
	}
	return left;
}
static treeNode *bool_or_exp()
{
	treeNode *left = bool_and_exp();

	while( token == TK_OR )
	{
		treeNode *node = newNode( EXP_OP );
		node->attr.ival = token;
		node->child[0] = left;
		match( token );
		node->child[1] = bool_and_exp();
		left = node;
	}

	return left;
}
static treeNode *bool_and_exp()
{
	treeNode *left = bool_equ_exp();

	while( token == TK_AND )
	{
		treeNode *node = newNode( EXP_OP );
		node->attr.ival = token;
		node->child[0] = left;
		match( token );
		node->child[1] = bool_equ_exp();
		left = node;
	}

	return left;
}
static treeNode *bool_equ_exp()
{
	treeNode *left = bool_rel_exp();

	while( token == TK_EQ || token == TK_NE )
	{
		treeNode *node = newNode( EXP_OP );
		node->attr.ival = token;
		node->child[0] = left;
		match( token );
		node->child[1] = bool_rel_exp();
		left = node;
	}

	return left;
}
static treeNode *bool_rel_exp()
{
	treeNode *left = simple_exp();

	while( token == TK_LT || token == TK_LE || token == TK_GT
		|| token == TK_GE )
	{
		treeNode *node = newNode( EXP_OP );
		node->attr.ival = token;
		node->child[0] = left;
		match( token );
		node->child[1] = simple_exp();
		left = node;
	}

	return left;
}
static treeNode *simple_exp()
{
	treeNode *left = term();

	while( token == TK_ADD || token == TK_SUB )
	{
		treeNode *node = newNode( EXP_OP );
		node->attr.ival = token;
		node->child[0] = left;
		match( token );
		node->child[1] = term();
		left = node;
	}

	return left;
}
static treeNode *term()
{
	treeNode *left = unary();

	while( token == TK_MUL || token == TK_DIV || token == TK_MOD )
	{
		treeNode *node = newNode( EXP_OP );
		node->attr.ival = token;
		node->child[0] = left;
		match( token );
		node->child[1] = unary();
		left = node;
	}

	return left;
}
static treeNode *unary()
{
	treeNode *t;

	switch( token )
	{
		case TK_NOT:
		case TK_BIT_AND:
		case TK_MUL:
			t = newNode( EXP_OP );
			t->attr.ival = token;
			match( token );
			t->child[1] = unary();
			break;
		case TK_SUB:
			t = newNode( EXP_OP );
			t->attr.ival = token;
			match( token );
			t->child[0] = newNode( EXP_CONST );
			t->child[0]->attr.ival = 0;
			t->child[1] = unary();
			break;
		default:
			t = factor();
	}

	return t;
}
static treeNode *factor()
{
	treeNode *t;

	switch( token )
	{
		case TK_RB_L:
			match( TK_RB_L );
			t = expression();
			match( TK_RB_R );
			break;
		case TK_ID:
			t = var();
			if( token == TK_RB_L )
			{
				t->type = EXP_FUN_CALL;
				match( TK_RB_L );
				t->child[0] = args();
				match( TK_RB_R );
			}
			break;
		case TK_NUM:
			t = newNode( EXP_CONST );
			t->attr.ival = atoi( tokenString );
			match( token );
			break;
		case TK_NUM_F:
			t = newNode( EXP_CONST_F );
			t->attr.fval = (float)atof( tokenString );
			match( token );
			break;
		case TK_NUM_HEX:
			t = newNode( EXP_CONST );
			sscanf( tokenString, "%x", &t->attr.ival );
			match( token );
			break;
		case TK_STR:
			t = newNode( EXP_STR );
			t->attr.sval = copyString( tokenString );
			match( token );
			break;
		case TK_CH:
			t = newNode( EXP_CONST );
			t->attr.ival = tokenString[0];
			match( token );
			break;
		default:
			t = NULL;
			throwError( lineno, "Illegal token -> '%s'", tokenToStr[token] );
			match( token );
			break;
	}

	return t;
}
static treeNode *var()
{
	treeNode *t = newNode( EXP_ID );

	t->attr.sval = copyString( tokenString );
	match( TK_ID );
	if( token == TK_SB_L )
	{
		treeNode **node = &t->child[0];

		do
		{
			match( TK_SB_L );
			*node = expression();
			match( TK_SB_R );

			node = &(*node)->sibling;
		}while( token == TK_SB_L );
	}

	return t;
}
static treeNode *args()
{
	treeNode *root, *prev;
	root = NULL;

	while( token != TK_RB_R )
	{
		treeNode *node = expression();

		if( node != NULL )
		{
			if( root == NULL )
				root = prev = node;
			else
			{
				prev->sibling = node;
				prev = node;
			}
		}

		if( token != TK_RB_R )
			match( TK_COM );
		if( unmatched )
		{
			unmatched = FALSE;
			break;
		}
	}

	return root;
}

/* free a typeInfo struct */
void freeTypeInfo( void )
{
	while( --top >= 0 )
		free( typeinfoP[top] );
}

/* free a syntax tree */
void freeTree( treeNode *tree )
{
	int i, type;

	if( tree == NULL )
		return;

	type = tree->type;
	if( type == SM_VAR_DEF || type == SM_FUN_DEF ||	type == SM_PARAM_DEF
		|| type == EXP_ID || type == EXP_FUN_CALL || type == EXP_STR )
			free( tree->attr.sval );

	for( i = 0; i < MAX_CHILDREN; ++i )
		freeTree( tree->child[i] );
	freeTree( tree->sibling );
	free( tree );

}