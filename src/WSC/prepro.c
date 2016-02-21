/********************************************************/
/* File: prepro.c                                       */
/* The preprocessor implementation for the WSC compiler */
/********************************************************/

#include "globals.h"
#include "scanner.h"
#include "parser.h"
#include "lib.h"

char *strrep( char *src, char *oldStr, char *newStr )
{
	tokenType token;

	while( (token = getToken()) != TK_EOF && token != TK_ERROR )
	{
		if( token == TK_ID || (token >= TK_IF && token <= TK_FLOAT) )
		{
			if( strcmp( tokenString, oldStr ) == 0 )
			{
				int pos = getPos();
				int diff = strlen( newStr ) - strlen( oldStr );
				int len = strlen( src );

				if( diff > 0 )
					src = (char *)realloc( src, len + diff + 1 );

				memmove( src + pos + diff, src + pos, len - pos );
				strncpy( src + pos - strlen( oldStr ), newStr, strlen( newStr ) );

				if( diff < 0 )
					src = (char *)realloc( src, len + diff + 1 );

				src[len + diff] = '\0';
				setScanner( src, lineno, pos + diff );
			}
		}
	}

	return src;
}

char *preprocessor( char *src, byteCodeInfo *info )
{
	tokenType token;
	int lineBegin, lineEnd; // to delete the preprocessor command

	setScanner( src, 1, 0 );

	while( (token = getToken()) != TK_EOF && token != TK_ERROR )
	{
		if( token == TK_POUND )
		{
			lineBegin = getPos() - 1;

			token = getToken();

			if( token == TK_ID )
			{
				if( strcmp( tokenString, "define" ) == 0 )
				{
					char s1[128], s2[128];
					char *pSrc, *pTar;

					token = getToken();
					if( token != TK_ID && (token < TK_IF || token > TK_FLOAT ) )
						throwError( lineno, "#define syntax :'%s' is no Identifier", tokenToStr[token] );

					strcpy( s1, tokenString );
					pSrc = src + getPos() + 1;
					pTar = s2;

					while( *pSrc && *pSrc != '\n'  )
					{
						*pTar = *pSrc;
						++pTar; ++pSrc;
					}
					*pTar = '\0';

					strcpy( src + lineBegin, pSrc ); // delete this line

					setScanner( src, lineno, lineBegin );
					src = strrep( src, s1, s2 );

					setScanner( src, lineno, lineBegin );
				}
				else if( strcmp( tokenString, "include" ) == 0 )
				{
					char *filename;

					if( (token = getToken()) != TK_LT )				// <
						throwError( lineno, "expect '<', buf found '%s'", tokenToStr[token] );
					if( (token = getToken()) != TK_ID )				// stdio.h
						throwError( lineno, "Expect a file name, buf found '%s'", tokenToStr[token] );
					else
						filename = copyString( tokenString );
					if( (token = getToken()) != TK_GT )				// >
						throwError( lineno, "Expect '>', buf found '%s'", tokenToStr[token] );

					lineEnd = getPos();
					strcpy( src + lineBegin, src + lineEnd ); // delete this line

					setScanner( src, lineno, lineBegin );
					loadLib( filename, &src, lineBegin );

					setScanner( src, lineno, lineBegin );
				}
				else if( strcmp( tokenString, "progma" ) == 0 )
				{
					token = getToken();

					if( strcmp( tokenString, "FORCE_BREAK" ) == 0 )
						info->forceBreak = TRUE;
					else if( strcmp( tokenString, "RAM_SIZE" ) == 0 )
					{
						if( (token = getToken()) == TK_NUM )
							sscanf( tokenString, "%d", &info->RAMSize );
						else if( token == TK_NUM_HEX )
							sscanf( tokenString, "%x", &info->RAMSize );
						else
							throwError( lineno, "Expect a number, buf found '%s'",
								tokenToStr[token] );
					}
					else if( strcmp( tokenString, "BIG_FONT" ) == 0 )
						info->bigFont = TRUE;
					else
						throwError( lineno, "Invalid progma command '%s'",
							tokenToStr[token] );

					lineEnd = getPos();
					strcpy( src + lineBegin, src + lineEnd ); // delete this line
					setScanner( src, lineno, lineBegin );
				}
				else
					throwError( lineno, "Invalid preprocessor command '%s'",
						tokenToStr[token] );
			}
			else
				throwError( lineno, "Invalid preprocessor command '%s'",
					tokenToStr[token] );
		}
	}

	return src;
}
