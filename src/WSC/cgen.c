/****************************************************/
/* File: cgen.c                                     */
/* The code generator implementation                */
/* for the WSC compiler                             */
/* (generates code for the FVM machine)             */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "emiter.h"
#include "cgen.h"
#include "lib.h"

static int scope;
static int lastWhile; /* for break statement */

/* for backpatching function call */
typedef struct funJump
{
	char *name;
	int saveLoc;
	struct funJump *next;
} funJump;

static funJump funBack = { 0 };
static funJump *now = &funBack;

/* prototype for internal recursive code generator */
static void gen( treeNode *node );
static void genNode( treeNode *node );

/* type to code table */
static opCode LD[] = { opLDL, opLDB, opLDF, opLDL, opLDL };
static opCode ST[] = { opSTL, opSTB, opSTF, opSTL, opSTL };
static opCode POP[] ={ opPOP, opPOP, opPOPF, opPOP, opPOP };
static opCode PUSH[]={ opPUSH, opPUSH, opPUSHF, opPUSH, opPUSH };
static opCode MOVE[]={ opMOV, opMOV, opMOVF, opMOV, opMOV };

/* frameOffset stack for funcall */
#define STACK_SIZE  128
static int frameOffsetStack[STACK_SIZE];
static int frameOffsetTop = 0;
static int frameOffset = 0;

/* for saving temp to reg */
#define MINTEMPREG 0xA
#define MAXTEMPREG 0xC
static int tempRegTop = MINTEMPREG;

static void pushFrameOffset( int val, int lineno )
{
	if( frameOffsetTop < STACK_SIZE )
	{
		frameOffsetStack[frameOffsetTop] = val;
		frameOffset += frameOffsetStack[frameOffsetTop-1];
		++frameOffsetTop;
	}
	else
		outOfMem( lineno, "frame offset static" );
}

static void popFrameOffset( void )
{
	--frameOffsetTop;
	frameOffset -= frameOffsetStack[frameOffsetTop-1];
}

/* add info for backpatching funcall statement */
static void addFunBack( const char *name, int lineno )
{
	now->next = (funJump *)malloc( sizeof(funJump) );
	if( now->next == NULL )
		outOfMem( lineno, "funcall jump" );
	else
	{
		now->name = (char *)name;
		now->saveLoc = emitSkip( 7 );
		now = now->next;
		now->next = NULL;
	}
}

/* backpatching break statement */
static void backpatchBreak( treeNode *node, int loc )
{
	if( node == NULL || node->type == SM_WHILE )/* only patch the nearest break */
		return;

	if( node->type == SM_BREAK )
	{
		emitBackup( node->attr.ival );
		emitAbs( opLA, $PC, loc );
		emitRestore();
	}
	else
	{
		int i;
		for( i = 0; i < MAX_CHILDREN; ++i )
			backpatchBreak( node->child[i], loc );
	}
	backpatchBreak( node->sibling, loc );
}

/* gen code for saving temp */
int genSaveTemp( char type, char isOptimizable )
{
	if( tempRegTop > MAXTEMPREG || !isOptimizable )
	{
		emitCode( PUSH[type], $AC, 0, 0 ); // save it to stack memory
		return FALSE;
	}
	else
	{
		emitCode( MOVE[type], tempRegTop++, $AC, 0 ); // save it to reg
		return TRUE;
	}
}
void genLoadTemp( char type, int reg, char isOptimizable )
{
	if( isOptimizable )
		--tempRegTop; // load from reg
	else
		emitCode( POP[type], reg, 0, 0 ); // load from stack memory
}


/* generates store statement */
static void genSto( treeNode *node )
{
	int base, offset, val, index;
	char type, type2;
	char valConst, indexConst;
	Bucket *bp;

	if( node->child[0]->type == EXP_OP )
		bp = searchDouble( node->child[0]->child[1]->attr.sval, scope, GLOBAL_SCOPE );
	else
		bp = searchDouble( node->child[0]->attr.sval, scope, GLOBAL_SCOPE );

	if( bp->scope == 0 )
	{ base = $GP; offset = bp->loc; }
	else
	{ base = $FP; offset = bp->loc + initFO; }

	// for code optimization
	if( node->child[1]->type == EXP_CONST )
	{ valConst = TRUE; val = node->child[1]->attr.ival; }
	else
	{ valConst = FALSE; genNode( node->child[1] ); }

	if( node->child[0]->child[0] != NULL )
	{
		if( node->child[0]->child[0]->type == EXP_CONST )
		{ indexConst = TRUE; index = node->child[0]->child[0]->attr.ival; }
		else
		{ indexConst = FALSE; }
	}

	if( bp->obj.type.t == VAR_ARRAY )
	{
		type = bp->obj.type.n->t;
		if( valConst )
		{
			if( indexConst )
			{
				/* get right operand */
				emitLC( $AC, val );
				/* assign */
				emitCode( ST[type], $AC, offset + index * SIZEOF[type], base );
			}
			else
			{
				/* gen code for index */
				genNode( node->child[0]->child[0] );
				/* ****************** */
				emitCode( opLCB, $AC1, SIZEOF[type], 0 );
				emitCode( opMUL, $AC1, $AC, $AC1 );
				/******************** */
				/* compute base */
				emitCode( opADD, $AC1, $AC1, base );
				/* get right operand */
				emitLC( $AC, val );
				/* assign */
				emitCode( ST[type], $AC, offset, $AC1 );
			}
		}
		else
		{
			if( indexConst )
				/* assign */
				emitCode( ST[type], $AC, offset + index * SIZEOF[type], base );
			else
			{
				type2 = node->child[1]->dataType.t;
				/* save right operand */
				emitCode( PUSH[type2], $AC, 0, 0 );
				/* gen code for index */
				genNode( node->child[0]->child[0] );
				/* ****************** */
				emitCode( opLCB, $AC1, SIZEOF[type], 0 );
				emitCode( opMUL, $AC1, $AC1, $AC );
				/******************** */
				/* compute base */
				emitCode( opADD, $AC1, $AC1, base );
				/* pop right operand */
				emitCode( POP[type2], $AC, 0, 0 );
				/* assign */
				emitCode( ST[type], $AC, offset, $AC1 );
			}
		}
	}
	else if( bp->obj.type.t == VAR_POINTER )
	{
		treeNode *t = node->child[0];
		type = bp->obj.type.n->t;
		if( t->child[0] == NULL )
		{
			if( t->attr.ival != TK_MUL )// is not a references, assign to pointer
			{
				if( valConst )
					emitLC( $AC, val );
				emitCode( opSTL, $AC, offset, base );
			}
			else // dereference
			{ 
				if( valConst )
					emitLC( $AC, val );
				/* get address(value of pointer) */
				emitCode( opLDL, $AC1, offset, base );
				/* assign */
				emitCode( ST[type], $AC, 0, $AC1 );
			}
		}
		else
		{
			if( valConst )
			{
				if( indexConst )
				{
					/* get address(value of pointer) */
					emitCode( opLDL, $AC1, offset, base );
					/* get right operand */
					emitCode( LD[node->child[1]->dataType.t], $AC, val, 0 );
					/* assign */
					emitCode( ST[type], $AC, index * SIZEOF[type], $AC1 );
				}
				else
				{
					/* gen code for index */
					genNode( t->child[0] );
					/* ****************** */
					emitCode( opLCB, $AC1, SIZEOF[type], 0 );
					emitCode( opMUL, $AC1, $AC1, $AC );
					/******************** */
					/* get address(value of pointer) */
					emitCode( opLDL, $AC, offset, base );
					/* compute base */
					emitCode( opADD, $AC1, $AC1, $AC );
					/* get right operand */
					emitLC( $AC, val );
					/* assign */
					emitCode( ST[type], $AC, 0, $AC1 );
				}
			}
			else
			{
				if( indexConst )
				{
					/* get address(value of pointer) */
					emitCode( opLDL, $AC1, offset, base );
					/* assign */
					emitCode( ST[type], $AC, index * SIZEOF[type], $AC1 );
				}
				else
				{
					type2 = node->child[1]->dataType.t;
					/* push right operand */
					emitCode( PUSH[type2], $AC, 0, 0 );
					/* gen code for index */
					genNode( t->child[0] );
					/* ****************** */
					emitCode( opLCB, $AC1, SIZEOF[type], 0 );
					emitCode( opMUL, $AC1, $AC1, $AC );
					/******************** */
					/* get address(value of pointer) */
					emitCode( opLDL, $AC, offset, base );
					/* compute base */
					emitCode( opADD, $AC1, $AC, $AC1 );
					/* pop right operand */
					emitCode( POP[type2], $AC, 0, 0 );
					/* assign */
					emitCode( ST[type], $AC, 0, $AC1 );
				}
			}
		}
	}
	else
	{
		if( valConst )
			emitLC( $AC, val );
		emitCode( ST[bp->obj.type.t], $AC, offset, base );
	}
}

/* generates '&'(get address) operation */
static void genGetAddress( treeNode *node )
{
	int base, offset;
	Bucket *bp = searchDouble( node->child[1]->attr.sval, scope, GLOBAL_SCOPE );

	if( bp->scope == 0 )
		base = $GP, offset = bp->loc;
	else
		base = $FP, offset = bp->loc + initFO;

	if( node->child[1]->child[0] != NULL )
	{
		typeInfo *type = &bp->obj.type;

		// like &a[5]
		while( type->t == VAR_ARRAY || type->t == VAR_POINTER )
			type = type->n;

		if( bp->obj.type.t == VAR_ARRAY )
		{
			/* gen code for index */
			genNode( node->child[1]->child[0] );
			/* ****************** */
			emitCode( opLCB, $AC1, SIZEOF[type->t], 0 );
			emitCode( opMUL, $AC, $AC1, $AC );
			/******************** */
			// base + AC(index) + offset
			emitCode( opLA, $AC1, offset, base );
			emitCode( opADD, $AC, $AC1, $AC );
		}
		else if( bp->obj.type.t == VAR_POINTER )
		{
			/* gen code for index */
			genNode( node->child[1]->child[0] );
			/* ****************** */
			emitCode( opLCB, $AC1, SIZEOF[type->t], 0 );
			emitCode( opMUL, $AC, $AC1, $AC );
			/* get address(value of pointer) */
			emitCode( opLDL, $AC1, offset, base );
			/* value of pointer + index */
			emitCode( opADD, $AC, $AC, $AC1 );
		}
	}
	else
		emitCode( opLA, $AC, offset, base );
}

static void emitJump( int isFloat, int left, opCode flag )
{
	emitCode( isFloat ? opFSUB : opSUB, $AC, left, $AC );
	emitJmp( isFloat ? opCJF : opCJ,
		flag, $AC, 10, $PC );         /*  length  */
	emitCode( opLCB, $AC, FALSE, 0 ); /*  3       */
	emitCode( opLA, $PC, 3, $PC );    /*  7       */
	emitCode( opLCB, $AC, TRUE, 0 );  /*  3       */
}

/* generates code at a tree node */
static void genNode( treeNode *node )
{
	char type;

	switch( node->type )
	{
		case SM_VAR_DEF:
			gen( node->child[MAX_CHILDREN-1] );
			break;
		case SM_IF:
			{
				int savedLoc1, savedLoc2, curLoc;

				/* generate code for test expression */
				genNode( node->child[0] );
				savedLoc1 = emitSkip( 8 );
				/* recurse on then part */
				gen( node->child[1] );
				savedLoc2 = emitSkip( 7 );
				curLoc = emitSkip( 0 );
				emitBackup( savedLoc1 );
				emitJmpAbs( opCJ, CJ_EQ, $AC, curLoc );
				emitRestore();
				/* recurse on else part */
				gen( node->child[2] );
				curLoc = emitSkip( 0 );
				emitBackup( savedLoc2 );
				emitAbs( opLA, $PC, curLoc );
				emitRestore();
			}
			break;
		case SM_WHILE:
			{
				int savedLoc1, savedLoc2, curLoc;

				savedLoc1 = emitSkip( 0 );
				/* generate code for test */
				genNode( node->child[0] );
				savedLoc2 = emitSkip( 8 );
				/* generate code for body */
				gen( node->child[1] );
				/* unconditional jump */
				emitAbs( opLA, $PC, savedLoc1 );
				curLoc = emitSkip( 0 );
				emitBackup( savedLoc2 );
				emitJmpAbs( opCJ, CJ_EQ, $AC, curLoc );
				emitRestore();
				/* backpatching break statement */
				backpatchBreak( node->child[1], curLoc );
			}
			break;
		case SM_RETURN:
			gen( node->child[0] ); // Don't use genNode(), node->child[0] may be NULL;
			/* load return address */
			emitCode( opLDL, $PC, retFO, $FP );
			break;
		case SM_BREAK:
			/* store loc to ival for backpatching */
			node->attr.ival = emitSkip( 7 );
			break;
		case EXP_OP:
			{
				int left;
				char isOptimizable;

				if( node->attr.ival == TK_STO )
				{
					genSto( node );
					break;
				}
				if( node->attr.ival == TK_BIT_AND && node->child[0] == NULL )
				{
					genGetAddress( node );
					break;
				}

				if( node->child[0] != NULL )
				{
					char nodeType = node->child[1]->type;
					type = node->child[0]->dataType.t;
					/* use the type of child[0], because the type of some operations
					 * (like '<', '>=', '==', '!=', '&&', etc) is VAR_INT, thought
					 * the operand is float.
					 */

					if( node->child[0]->type == EXP_FUN_CALL ||
						node->child[1]->type == EXP_FUN_CALL )
						isOptimizable = FALSE;
					else
						isOptimizable = TRUE;

					/* gen code for left operand */
					genNode( node->child[0] );
					/* save left operand */
					isOptimizable = genSaveTemp( type, isOptimizable );
					/* gen code for right operand */
					genNode( node->child[1] );
					/* load left operand */
					genLoadTemp( type, $AC1, isOptimizable );
				}
				else
					/* gen code for right operand */
					genNode( node->child[1] );

				if( isOptimizable )
					left = tempRegTop;
				else
					left = $AC1;

				switch( node->attr.ival )
				{
					case TK_ADD: emitCode( type == VAR_FLOAT ? opFADD: opADD,
									 $AC, left, $AC ); break;
					case TK_SUB: emitCode( type == VAR_FLOAT ? opFSUB: opSUB,
									 $AC, left, $AC ); break;
					case TK_MUL:
						if( node->child[0] != NULL )
							emitCode( type == VAR_FLOAT ? opFMUL: opMUL,
									 $AC, left, $AC );
						else // pointer
							emitCode( LD[node->child[1]->dataType.n->t], $AC, 0, $AC );
						break;
					case TK_DIV:emitCode( type == VAR_FLOAT ? opFDIV: opDIV,
									 $AC, left, $AC ); break;
					case TK_MOD:
						/* a mod b == a - a/b*b */
						emitCode( opDIV, $AC2, left, $AC );
						emitCode( opMUL, $AC2, $AC2, $AC );
						emitCode( opSUB, $AC, left, $AC2 );
						break;
					case TK_LT: emitJump( type == VAR_FLOAT, left, CJ_LT ); break;
					case TK_LE: emitJump( type == VAR_FLOAT, left, CJ_LE ); break;
					case TK_GT: emitJump( type == VAR_FLOAT, left, CJ_GT ); break;
					case TK_GE: emitJump( type == VAR_FLOAT, left, CJ_GE ); break;
					case TK_EQ: emitJump( type == VAR_FLOAT, left, CJ_EQ ); break;
					case TK_NE: emitJump( type == VAR_FLOAT, left, CJ_NE ); break;
					case TK_AND:
						type = ( type == VAR_FLOAT ? opCJF : opCJ ); 
						emitJmp( type, CJ_EQ, left, 18, $PC );/*  length  */
						emitJmp( type, CJ_EQ, $AC, 10, $PC ); /*  8       */
						emitCode( opLCB, $AC, TRUE, 0 );      /*  3       */
						emitCode( opLA, $PC, 3, $PC );        /*  7       */
						emitCode( opLCB, $AC, FALSE, 0 );     /*  3       */
						break;
					case TK_OR:
						type = ( type == VAR_FLOAT ? opCJF : opCJ ); 
						emitJmp( type, CJ_NE, left, 18, $PC );/*  length  */
						emitJmp( type, CJ_NE, $AC, 10, $PC ); /*  8       */
						emitCode( opLCB, $AC, FALSE, 0 );     /*  3       */
						emitCode( opLA, $PC, 3, $PC );        /*  7       */
						emitCode( opLCB, $AC, TRUE, 0 );      /*  3       */
						break;
					case TK_NOT:
						emitJmp( type == VAR_FLOAT ? opCJF : opCJ,
							CJ_EQ, $AC, 10, $PC );            /*  length  */
						emitCode( opLCB, $AC, FALSE, 0 );     /*  3       */
						emitCode( opLA, $PC, 3, $PC );        /*  7       */
						emitCode( opLCB, $AC, TRUE, 0 );      /*  3       */
					default : break;
				}
			}
			break;
		case EXP_CONST:
			emitLC( $AC, node->attr.ival );
			break;
		case EXP_CONST_F:
			emitLCF( $AC, node->attr.fval );
			break;
		case EXP_ID:
			{
				Bucket *bp = searchDouble( node->attr.sval, scope, GLOBAL_SCOPE );
				int base, offset;

				if( bp->scope == 0 ) // global
				{ base = $GP; offset = bp->loc; }
				else
				{ base = $FP; offset = bp->loc + initFO; }

				if( bp->obj.type.t == VAR_ARRAY )
				{
					if( node->child[0] == NULL )
						emitCode( opLA, $AC, offset, base );
					else
					{
						type = bp->obj.type.n->t;
						/* gen code for index */
						genNode( node->child[0] );
						/* ****************** */
						emitCode( opLCB, $AC1, SIZEOF[type], 0 );
						emitCode( opMUL, $AC, $AC1, $AC );
						/******************** */
						/* compute base */
						emitCode( opADD, $AC, $AC, base );
						/* get value */
						emitCode( LD[type], $AC, offset, $AC );
					}
				}
				else if( bp->obj.type.t == VAR_POINTER )
				{
					if( node->child[0] == NULL )
						emitCode( opLDL, $AC, offset, base );
					else
					{
						type = bp->obj.type.n->t;
						/* gen code for index */
						genNode( node->child[0] );
						/* ****************** */
						emitCode( opLCB, $AC1, SIZEOF[type], 0 );
						emitCode( opMUL, $AC, $AC1, $AC );
						/******************** */
						/* get address(value of pointer) */
						emitCode( opLDL, $AC1, offset, base );
						/* change base */
						emitCode( opADD, $AC, $AC, $AC1 );
						/* get value */
						emitCode( LD[type], $AC, 0, $AC );
					}
				}
				else
					emitCode( LD[bp->obj.type.t], $AC, offset, base );
			}
			break;
		case EXP_FUN_CALL:
			{
				treeNode *t;
				Bucket *bp = searchDouble( node->attr.sval, LIBFUN_SCOPE, USERFUN_SCOPE );
				int i;

				pushFrameOffset( bp->obj.val.fval.frameSize + initFO, node->lineno );

				/* compute arguments */
				if( bp->scope == USERFUN_SCOPE ) // uesr's function
				{
					treeNode *args = bp->obj.val.fval.address->child[0];

					for( t = node->child[0], i = 0; t != NULL; t = t->sibling,
														args = args->sibling )
					{
						genNode( t );
						/* save it to new frame */
						emitCode( ST[args->dataType.t], $AC, initFO + frameOffset + i, $FP );
						i += SIZEOF[args->dataType.t];
					}
				}
				else // lib's function
				{
					char *args = ((libFun *)bp->obj.val.fval.address)->args;

					for( t = node->child[0], i = 0; t != NULL; t = t->sibling, ++args )
					{
						genNode( t );
						/* save it to new frame */
						emitCode( ST[*args], $AC, initFO + frameOffset + i, $FP );
						i += SIZEOF[*args];
					}
				}
				/* push current fp */
				emitCode( opSTL, $FP, frameOffset + ofpFO, $FP );
				/* push new frame */
				emitCode( opLA, $FP, frameOffset, $FP );
				/* save return in ac */
				emitCode( opLA, $AC, 7, $PC );
				/* relative jump to function entry */
				addFunBack( node->attr.sval, node->lineno );
				/* pop current frame */
				emitCode( opLDL, $FP, ofpFO, $FP );

				popFrameOffset();
			}
			break;
		case EXP_CAST:
			genNode( node->child[0] );
			switch( node->attr.ival )
			{
				case CAST_FI:
					emitCode( opFI, $AC, $AC, 0 );
				case CAST_IF:
					emitCode( opIF, $AC, $AC, 0 );
				default:
					break;
			}
			break;
		default:
			break;
	}
}

static void gen( treeNode *node )
{
	if( node == NULL )
		return;

	genNode( node );

	gen( node->sibling );
}

/* for backpatching function call */
static void backpatchFuncall( void )
{
	for( now = &funBack; now->next != NULL; now = now->next )
	{
		Bucket *bp = searchDouble( now->name, LIBFUN_SCOPE, USERFUN_SCOPE );

		if( bp->obj.type.t == VAR_FUN )
		{
			emitBackup( now->saveLoc );
			emitAbs( opLA, $PC, bp->loc );
			emitRestore();
		}
		else
		{
			/* link library function */
			int curLoc = emitSkip( 0 );
			libFun *fun = (libFun*)bp->obj.val.fval.address;

			emitBackup( now->saveLoc );
			emitAbs( opLA, $PC, curLoc );
			emitRestore();

			/* store return address */
			emitCode( opSTL, $AC, retFO, $FP );
			/* store argument address */
			emitCode( opLA, $IA, initFO, $FP );
			emitCode( opINT, fun->vector, fun->funCode, 0 );
			/* load return address */
			emitCode( opLDL, $PC, retFO, $FP );

			/* linked */
			bp->loc = curLoc;
			bp->obj.type.t = VAR_FUN;
		}
	}
}
static void freeFunback()
{
	funJump *temp;

	now = funBack.next;
	while( now != NULL )
	{
		temp = now->next;
		free( now );
		now = temp;
	}
}


/**********************************************/
/* the primary function of the code generator */
/**********************************************/
/* generates code by traversal of the syntax tree. */
void codeGen( treeNode *root, unsigned char *output, unsigned char *data,
			byteCodeInfo *info )
{
	treeNode *node;

	initEmiter( output );
	emitHeader( data, info );
	pushFrameOffset( 0, -1 );

	/* generate standard prelude */
	emitCode( opLCB, $GP, 0, 0 );
	emitLC( $FP, info->globalSize );

	/* generate code for global var init */
	for( node = root; node != NULL; node = node->sibling )
	{
		if( node->type == SM_VAR_DEF && node->child[MAX_CHILDREN-1]
			!= NULL )
			gen( node->child[MAX_CHILDREN-1] );
	}

	/* call main function */
	emitCode( opLA, $AC, 7, $PC ); // save return in ac
	addFunBack( "main", 0 );
	emitCode( opHALT, 0, 0, 0 );   // end

	/* generate code for function */
	scope = 0;
	for( node = root; node != NULL; node = node->sibling )
	{
		if( node->type == SM_FUN_DEF )
		{
			Bucket *bp = symtabLookup( node->attr.sval, USERFUN_SCOPE );

			++scope;
			pushFrameOffset( bp->obj.val.fval.frameSize + initFO,
				node->lineno);
			bp->loc = emitSkip( 0 );
			/* store return address */
			emitCode( opSTL, $AC, retFO, $FP );
			/* gen body */
			gen( node->child[1] );
			/* load return address */
			emitCode( opLDL, $PC, retFO, $FP );
			popFrameOffset();
		}
	}

	backpatchFuncall();
	freeFunback();
	info->codeSize = emitSkip( 0 );
}