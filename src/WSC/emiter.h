/****************************************************/
/* File: emiter.h                                   */
/* Code emitting utilities for the WSC compiler     */
/* and interface to the FVM machine                 */
/****************************************************/

#ifndef _EMITER_H_
#define _EMITER_H_

#include "fvm.h"

/* FO = frame offset */
#define ofpFO (0)
#define retFO (4)
#define initFO (8)

/* accumulator */
#define $AC		(0x00)

/* 2nd accumulator */
#define $AC1	(0x01)

/* 3rd accumulator */
#define $AC2	(0x02)

/* fp = "frame pointer" points
 * to top of stace frame
 */
#define $FP		(0x03)

/* gp = "global pointer" points
 * to top of global variable memory
 */
#define $GP		(0x04)

/* set the input of emiter and init the emiter */
void initEmiter( unsigned char *output );

/* emits header and static section of a program */
void emitHeader( unsigned char *data, byteCodeInfo *info );

/* emits a FVM instruction */
void emitCode( opCode op, int arg1, int arg2, int arg3 );
void emitJmp( opCode op, int arg1, int arg2, int arg3, int arg4 );
void emitLC( int arg1, int arg2 );
void emitLCF( int arg1, float arg2 );

/* converts an absolute reference 
 * to a pc-relative reference when emitting a
 * register-to-memory FVM instruction
 * op = the opcode
 * r = target register
 * a = the absolute location in memory
 */
void emitAbs( opCode op, int r, int a );
void emitJmpAbs( opCode op, int flag, int r, int a );

/* skips "howMany" code locations for later backpatch. 
 * It also returns the current code position
 */
int emitSkip( int howMany );

/* backs up to loc = a previously skipped location */
void emitBackup( int loc );

/* restores the current code position to the highest
 * previously unemitted position
 */
void emitRestore( void );

#endif