#ifndef _FVM_H_
#define _FVM_H_

typedef enum
{
	opNOP = 0,
	/* data transfer */
	opLCB,opLCL,opLCF,		/* load constant */
	opLA,					/* load address */
	opLDB,opLDL,opLDF,		/* load from memory */
	opSTB,opSTL,opSTF,		/* store to memory */
	opMOV,opMOVF,
	/* flow control */
	opCJ,opCJF,opINT,opHALT, // CJ == Conditional Jump
	/* arithmetic */
	opADD,opSUB,opMUL,opDIV,opFADD,opFSUB,opFMUL,opFDIV,
	/* conversion */
	opFI,opIF,
	/* stack */
	opPUSH, opPUSHF, opPOP, opPOPF,
}opCode;
extern const char *codeToStr[];

typedef enum {
	srOKAY,
	srHALT,
	srERROR,
} stepResult;

/* REGISTERS ------------------------------------------------------*/
#define $TP		0xD	/* bottom of stack */
#define $IA		0xE /* interrupt argument */
#define $PC		0xF	/* address of next instruction to execute */

/* General purpose registers */
#define R_SIZE 16
extern int R[];
extern const char *R_STR[];

#define Rf_SIZE 16
extern float Rf[];
extern const char *Rf_STR[];

/* header of file */
#define HEADER_SIZE		16
/*    HEADER OF PROGRAM   */
/* 0:     MAGIC1          */
/* 1:     MAGIC2          */
/* 2~5:   RAM SIZE        */
/* 6~9:   STATIC SIZE     */
/* 13:    BIG_FONT        */
/* 14:	  FORCE_BREAK     */
#define APP_MAGIC1		0xFF
#define APP_MAGIC2		0x5D
#define DEFAULT_RAMSIZE			8 << 10 /* 8KB = 8,192 bytes */

/* unconditional jump code */
#define CJ_EQ			0x1
#define CJ_NE			0x2
#define CJ_LT			0x3
#define CJ_LE			0x4
#define CJ_GT			0x5
#define CJ_GE			0x6

/******** interrupt vector ********/
/* stdio.h */
#define INT_IO			0x01
#define INT_IO_PRINT	0x01
#define INT_IO_PRINT_D	0x02
#define INT_IO_PRINT_F	0x03
#define INT_IO_PRINT_S	0x04
#define INT_IO_SCAN		0x05
#define INT_IO_SCAN_S	0x06
#define INT_IO_GETSN	0x07

#define INT_IO_SPRINT	0x08
#define INT_IO_SPRINT_D	0x09
#define INT_IO_SPRINT_F	0x0A
#define INT_IO_SPRINT_S	0x0B
#define INT_IO_SSCAN	0x0C
#define INT_IO_SSCAN_S	0x0D
#define INT_IO_GETCHAR	0x0E
#define INT_IO_PUTCHAR	0x0F

/* ctype.h */
#define INT_CT			0x02
#define INT_CT_ISALNUM	0x01
#define INT_CT_ISALPHA	0x02
#define INT_CT_ISCNTRL	0x03
#define INT_CT_ISDIGIT	0x04
#define INT_CT_ISGRAPH	0x05
#define INT_CT_ISLOWER	0x06
#define INT_CT_ISPRINT	0x07
#define INT_CT_ISPUNCT	0x08
#define INT_CT_ISSPACE	0x09
#define INT_CT_ISUPPER	0x0A
#define INT_CT_ISXDIGIT	0x0B
#define INT_CT_TOLOWER	0x0C
#define INT_CT_TOUPPER	0x0D

/* mathf.h */
#define INT_MAF			0x03
#define INT_MAF_ACOS	0x01
#define INT_MAF_ASIN	0x02
#define INT_MAF_ATAN	0x03
#define INT_MAF_ATAN2	0x04
#define INT_MAF_COS		0x05
#define INT_MAF_SIN		0x06
#define INT_MAF_TAN		0x07
#define INT_MAF_COSH	0x08
#define INT_MAF_SINH	0x09
#define INT_MAF_TANH	0x0A
#define INT_MAF_EXP		0x0B
#define INT_MAF_FREXP	0x0C
#define INT_MAF_LDEXP	0x0D
#define INT_MAF_LOG		0x0E
#define INT_MAF_LOG10	0x0F
#define INT_MAF_MODF	0x10
#define INT_MAF_POW		0x11
#define INT_MAF_SQRT	0x12
#define INT_MAF_CEIL	0x13
#define INT_MAF_FABS	0x14
#define INT_MAF_FLOOR	0x15
#define INT_MAF_FMOD	0x16

/* stdlib.h */
#define INT_SL			0x04
#define INT_SL_ATOF		0x01
#define INT_SL_ATOI		0x02
#define INT_SL_ATOL		0x03
#define INT_SL_RAND		0x04
#define INT_SL_SRAND	0x06
#define INT_SL_CALLOC	0x07
#define INT_SL_FREE		0x08
#define INT_SL_MALLOC	0x09
#define INT_SL_REALLOC	0x0A
#define INT_SL_ABS		0x0B
#define INT_SL_LABS		0x0C

/* string.h */
#define INT_STR			0x05
#define INT_STR_MEMCPY	0x01
#define INT_STR_STRCPY	0x02
#define INT_STR_STRNCPY	0x03
#define INT_STR_STRCAT	0x04
#define INT_STR_STRNCAT	0x05
#define INT_STR_MEMCMP	0x06
#define INT_STR_STRCMP	0x07
#define INT_STR_STRNCMP	0x08
#define INT_STR_MEMCHR	0x09
#define INT_STR_STRCHR	0x0A
#define INT_STR_STRCSPN 0x0B
#define INT_STR_STRPBRK 0x0C
#define INT_STR_STRRCHR	0x0D
#define INT_STR_STRSPN	0x0E
#define INT_STR_STRSTR	0x0F
#define INT_STR_MEMSET	0x10
#define INT_STR_STRERR	0x11
#define INT_STR_STRLEN	0x12
#define INT_STR_MEMMOVE	0x13


/***************** FXLIB.H *****************/
/* dispbios.h */
#define INT_DB				0x06
#define INT_DB_ALLCLR		0x01
#define INT_DB_AREACLR		0x02
#define INT_DB_AREAREV		0x03
#define INT_DB_GETDISP		0x04
#define INT_DB_PUTDISP		0x05
#define INT_DB_SETPOINT		0x06
#define INT_DB_GETPOINT		0x07
#define INT_DB_WRITEG		0x08
#define INT_DB_READAREA		0x09
#define INT_DB_DRAWLINE		0x0A
#define INT_DB_CLEARLINE 	0x0B
#define INT_DB_LOCATE		0x0C
#define INT_DB_PRINT		0x0D
#define INT_DB_PRINTXY		0x0E
#define INT_DB_PRINTMINI 	0x0F
#define INT_DB_SAVEDISP		0x10
#define INT_DB_RESTDISP		0x11
#define INT_DB_POPUPWIN		0x12
#define INT_DB_DRAWCIRCLE	0x13
#define INT_DB_FILLCIRCLE	0x14
#define INT_DB_DRAWBOX		0x15
#define INT_DB_FILLBOX		0x16

/* filebios.h */
#define INT_FB				0x07
#define INT_FB_OPENFILE		0x01
#define INT_FB_READFILE		0x02
#define INT_FB_WRITEFILE 	0x03
#define INT_FB_SEEKFILE		0x04
#define INT_FB_CLOSEFILE 	0x05
#define INT_FB_GETFREE		0x06
#define INT_FB_GETSIZE		0x07
#define INT_FB_CREATEFILE	0x08
#define INT_FB_CREATEDIR	0x09
#define INT_FB_DELETEFILE	0x0A
#define INT_FB_DELETEDIR	0x0B
#define INT_FB_FINDFIRST 	0x0C
#define INT_FB_FINDNEXT		0x0D
#define INT_FB_FINDCLOSE 	0x0E

/* keybios.h */
#define INT_KB				0x08
#define INT_KB_GETKEY		0x01
#define INT_KB_ISKEYDOWN 	0x02
#define INT_KB_WAITKEY		0x03

/* others system function */
#define INT_SYS				0x09
#define INT_SYS_SLEEP		0x01
#define INT_SYS_CPUSPEED	0x02
#define INT_SYS_RESETCALC	0x03
#define INT_SYS_READRTC		0x04
#define INT_SYS_SETRTC		0x05
#define INT_SYS_BITAND		0x06
#define INT_SYS_BITOR		0x07
#define INT_SYS_BITXOR		0x08
#define INT_SYS_BITNOT		0x09
#define INT_SYS_SHIFTL		0x0A
#define INT_SYS_SHIFTR		0x0B
#define INT_SYS_EXEFVM		0x0C
#define INT_SYS_GETFVMMSG	0x0D


#define CAST_INT(a)		(*(int *)(a))
#define CAST_FLOAT(a)	(*(float *)(a))

#endif