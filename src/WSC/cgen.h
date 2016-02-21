/****************************************************/
/* File: cgen.h                                     */
/* The code generator interface to the WSC compiler */
/****************************************************/

#ifndef _CGEN_H_
#define _CGEN_H_

/* generates code by traversal of the syntax tree. */
void codeGen( treeNode *root, unsigned char *output, unsigned char *data,
			byteCodeInfo *info );

#endif