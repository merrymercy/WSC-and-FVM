/****************************************************/
/* File: file.h                                     */
/****************************************************/

#ifndef _FILE_H_
#define _FILE_H_

#include "fxlib.h"

/* return a filename in c-style */
char *SelectFile (char *filename);

#define DEBUG
// if define DEBUG, the files are in SD card
// else the files are in storage memory

#ifdef DEBUG
#define ROOT "crd0"
#else
#define ROOT "fls0"
#endif

FONTCHARACTER *CharToFont( const char *cFileName, FONTCHARACTER *fFileName );
char *FontToChar( const FONTCHARACTER *fFileName, char *cFileName );

#endif
