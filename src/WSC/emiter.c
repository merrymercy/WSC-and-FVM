/****************************************************/
/* File: emiter.c                                   */
/* FVM Code emitting utilities                      */
/* implementation for the WSC compiler              */
/****************************************************/

#include "globals.h"
#include "emiter.h"

/* FVM location number for current instruction emission */
static int loc;

/* Highest FVM location emitted so far For use in 
 * conjunction with emitSkip, emitBackup, and emitRestore
 */
static int highLoc;

/* code buffer */
static unsigned char *codes;

/* set the input of emiter and init the emiter */
void initEmiter( unsigned char *output )
{
	loc = highLoc = 0;
	codes = output;
}

/* emits header and static section of a program */
void emitHeader( unsigned char *data, byteCodeInfo *info )
{
	int temp;

	codes[loc] = APP_MAGIC1;
	codes[loc+1] = APP_MAGIC2;

	if( info->RAMSize == 0 )
		temp = DEFAULT_RAMSIZE;
	else
		temp = info->RAMSize;
	memcpy( codes+loc+2, &temp, sizeof( int ) );
	memcpy( codes+loc+6, &info->staticSize, sizeof( int ) );
	codes[loc+13] = info->bigFont;
	codes[loc+14] = info->forceBreak;

	memcpy( codes + HEADER_SIZE, data, info->staticSize );

	loc += HEADER_SIZE + info->staticSize;
}

/* emits a FVM instruction */
void emitCode( opCode op, int arg1, int arg2, int arg3 )
{
	codes[loc] = op;
	switch( op )
	{
		case opNOP:		case opHALT:
			loc += 1;		break;
		case opLCB:
			codes[loc+1] = arg1; codes[loc+2] = arg2;
			loc += 3;		break;
		case opLCL:
			codes[loc+1] = arg1; memcpy( codes+loc+2, &arg2, sizeof(int) );
			loc += 6;		break;
		case opLA:
		case opLDB:		case opLDL:		case opLDF:
		case opSTB:		case opSTL:		case opSTF:
			codes[loc+1] = arg1; memcpy( codes+loc+2, &arg2, sizeof(int) );
			codes[loc+6] = arg3;
			loc += 7;		break;
		case opMOV:		case opMOVF:		case opINT:
			codes[loc+1] = arg1; codes[loc+2] = arg2;
			loc += 3;		break;
		case opADD:		case opSUB:		case opMUL:		case opDIV:
		case opFADD:	case opFSUB:	case opFMUL:	case opFDIV:
			codes[loc+1] = arg1; codes[loc+2] = arg2;
			codes[loc+3] = arg3;
			loc += 4;		break;
		case opFI:		case opIF:
			codes[loc+1] = arg1; codes[loc+2] = arg2;
			loc += 3;		break;
		case opPOP:		case opPOPF:	case opPUSH:	case opPUSHF:
			codes[loc+1] = arg1;
			loc += 2;		break;
	}
	if( loc > highLoc )
		highLoc = loc;
}
void emitJmp( opCode op, int arg1, int arg2, int arg3, int arg4 )
{
	codes[loc] = op;
	codes[loc+1] = arg1;
	codes[loc+2] = arg2;
	memcpy( codes+loc+3 , &arg3, sizeof(int) );
	codes[loc+7] = arg4;
	loc += 8;

	if( loc > highLoc )
		highLoc = loc;
}
void emitLC( int arg1, int arg2 )
{
	if( arg2 >= -127 && arg2 <= 127 )
		emitCode( opLCB, arg1, arg2, 0 );
	else
		emitCode( opLCL, arg1, arg2, 0 );
}
void emitLCF( int arg1, float arg2 )
{
	codes[loc] = opLCF;
	codes[loc+1] = arg1;
	memcpy( codes+loc+2, &arg2, sizeof(float) );
	loc += 6;

	if( loc > highLoc )
		highLoc = loc;
}

/* converts an absolute reference 
 * to a pc-relative reference when emitting a
 * register-to-memory FVM instruction
 * op = the opcode
 * r = target register
 * a = the absolute location in memory
 */
void emitAbs( opCode op, int r, int a )
{
	emitCode( op, r, a - (loc+7), $PC );
}
void emitJmpAbs( opCode op, int flag, int r, int a )
{
	emitJmp( op, flag, r, a - (loc+8), $PC );
}

/* skips "howMany" code locations for later backpatch. 
 * It also returns the current code position
 */
int emitSkip( int howMany )
{  
	int t = loc;

	loc += howMany ;

	if( highLoc < loc )
		highLoc = loc;
	return t;
}

/* backs up to loc = a previously skipped location */
void emitBackup( int back )
{
	loc = back;
}

/* restores the current code position to the highest
 * previously unemitted position
 */
void emitRestore( void )
{
	loc = highLoc;
}