/****************************************************/
/* File: scanner.h                                  */
/* The scanner interface for the WSC compiler       */
/****************************************************/

#ifndef _SCANNER_H_
#define _SCANNER_H_

/* MAXTOKENLEN is the maximum size of a token */
#define MAXTOKENLEN 256

/* tokenString array stores the lexeme of each token */
extern char tokenString[MAXTOKENLEN];

/* source line number for error report */
extern int lineno;

/* returns the next token in source file */
tokenType getToken(void);

/* return the position of source file */
int getPos( void );

/* set the environment variable of scanner */
void setScanner( char *in, int line, int beginPos );


#endif