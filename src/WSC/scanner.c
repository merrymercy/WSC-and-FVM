/****************************************************/
/* File: scanner.c                                  */
/* The scanner implementation for the WSC compiler  */
/****************************************************/

#include "globals.h"
#include "scanner.h"

/* states in scanner DFA */
enum
{
	START, INCOMMENT_C, INCOMMENT_CPP, INNUM, INNUM_F,
	INNUM_HEX, INID, INSTR, DONE
};

/* lexeme of identifier or keyword */
char tokenString[MAXTOKENLEN];

/* source line number for error report */
int lineno;

/* source code */
static char *src;
/* current position in source code */
static int pos;

/* lookup table of keywords */
static struct
{
	char* str;
	tokenType type;
}keywords[]=
{
	{"if", TK_IF}, {"else", TK_ELSE}, {"while", TK_WHILE},
	{"for", TK_FOR}, {"return", TK_RETURN}, {"break", TK_BREAK},
	{"void", TK_VOID}, {"int", TK_INT},	{"char", TK_CHAR },
	{"float", TK_FLOAT},
};
#define KEYWORD_SIZE (10)

/* set the environment variable of scanner */
void setScanner( char *in, int line, int beginPos )
{
	src = in;
	lineno = line;
	pos = beginPos;

}

/* return the position of source file */
int getPos( void )
{
	return pos;
}

/* lookup an identifier to see if it is a keyword */
static tokenType lookUpKw( void )
{
	int i;
	for( i = 0; i < KEYWORD_SIZE; ++i )
		if( strcmp( tokenString, keywords[i].str ) == 0 )
			return keywords[i].type;
	return TK_ID;
}

static char getNext( void )
{
	return src[pos++];
}

static void back( void )
{
	--pos;
}

/****************************************/
/* the primary function of the scanner  */
/****************************************/
/* returns the next token in source string */
tokenType getToken( void )
{
	char save;			/* flag to indicate save to tokenString */
	int index = 0;		/* index for storing into tokenString */
	tokenType type;		/* holds current token to be returned */
	char state = START;	/* current state - always begins at START */

	while( state != DONE )
	{
		int c = getNext();
		if( c == '\0' )
			state = START;
		save = TRUE;
		switch( state )
		{
			case START:
				if( isdigit( c ) )
				{
					state = INNUM;
					if( c == '0' )
					{
						if( (c = getNext()) == 'x' || c == 'X' )
						{
							save = FALSE;
							state = INNUM_HEX;
						}
						else
						{
							back();
							c = src[pos-1];
						}
					}
				}
				else if( isalpha( c ) || c == '_' )
					state = INID;
				else if( isspace( c ) )
				{
					save = FALSE;
					if( c == '\n' )
						++lineno;
				}
				else if( c == '/' )
				{
					c = getNext();

					if( c == '*' )
					{	save = FALSE; state = INCOMMENT_C;		}
					else if( c == '/' )
					{	save = FALSE; state = INCOMMENT_CPP;	}
					else if( c == '=' )
					{	type = TK_DIV_; state = DONE;			}
					else
					{	type = TK_DIV; state = DONE; back();	}
				}
				else if( c == '"' )
				{	save = FALSE; state = INSTR;	}
				else if( c == '\'' )
				{
					c = getNext();

					if( c == '\\' )
					{
						c = getNext();
						switch( c )
						{
							case '"': case '\'': case '\\': break;
							case 'n': c = '\n'; break;
							case 't': c = '\t'; break;
							default: back(); break;
						}
					}

					tokenString[index++] = c;
					state = DONE;
					c = getNext();
					if( c == '\'' )
						type = TK_CH;
					else
					{	type = TK_ERROR; back();	}
				}


#define DOUBLE( c1, c2, t1, t2 )	else if( c == c1 ){\
									c = getNext();\
									state = DONE;\
									if( c == c2 ) type = t1;\
									else{ type = t2; back();}}

				DOUBLE( '<', '=', TK_LE, TK_LT )
				DOUBLE( '>', '=', TK_GE, TK_GT )
				DOUBLE( '=', '=', TK_EQ, TK_STO )
				DOUBLE( '&', '&', TK_AND, TK_BIT_AND )
				DOUBLE( '|', '|', TK_OR, TK_ERROR )
				DOUBLE( '!', '=', TK_NE, TK_NOT )
				DOUBLE( '+', '=', TK_ADD_, TK_ADD )
				DOUBLE( '-', '=', TK_SUB_, TK_SUB )
				DOUBLE( '*', '=', TK_MUL_, TK_MUL )
				DOUBLE( '%', '=', TK_MOD_, TK_MOD )
				else
				{
					state = DONE;
					switch( c )
					{
						case '\0' : type = TK_EOF; break;
						case ';' : type = TK_SEM; break;
						case ',' : type = TK_COM; break;
						case '(' : type = TK_RB_L; break;
						case ')' : type = TK_RB_R; break;
						case '[' : type = TK_SB_L; break;
						case ']' : type = TK_SB_R; break;
						case '{' : type = TK_CB_L; break;
						case '}' : type = TK_CB_R; break;
						case '#' : type = TK_POUND; break;
						default: type = TK_ERROR; break;
					}
				}
				break;
			case INCOMMENT_C:
				save = FALSE;
				if( c == '*' )
				{
					c = getNext();
					if( c == '/' )
						state = START;
					else
						back();
				}
				else if( c == '\n' )
					++lineno;
				break;
			case INCOMMENT_CPP:
				save = FALSE;
				if( c == '\n' )
				{
					++lineno;
					state = START;
				}
				break;
			case INNUM:
				if( !isdigit( c ) )
				{
					if( c == '.' )
						state = INNUM_F;
					else
					{
						state = DONE;
						type = TK_NUM;
						back();
					}
				}
				break;
			case INNUM_F:
				if( !isdigit( c ) )
				{
					state = DONE;
					type = TK_NUM_F;
					back();
				}
				break;
			case INNUM_HEX:
				if( !isxdigit( c ) )
				{
					state = DONE;
					type = TK_NUM_HEX;
					back();
				}
				break;
			case INID:
				if( !isalpha( c ) && !isdigit(c) && c != '_' && c != '.' )
				{
					state = DONE;
					type = TK_ID;
					back();
				}
				break;
			case INSTR:
				if( c == '"' )
				{
					state = DONE;
					type = TK_STR;
				}
				else if( c == '\\' )
				{
					c = getNext();
					switch( c )
					{
						case '"': case '\'': case '\\': break;
						case 'n': c = '\n'; break;
						case 't': c = '\t'; break;
						default: back(); break;
					}
				}
				break;
			default:break;
		}
		if( state == DONE )
			break;

		if( save && index < MAXTOKENLEN )
			tokenString[index++] = c;
	}

	tokenString[index] = '\0';
	if( type == TK_ID )
		type = lookUpKw();

	return type;
}
