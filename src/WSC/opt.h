/****************************************************/
/* File: opt.h                                      */
/* code optimization utilities                      */
/* interface for the FLOY compiler                  */
/****************************************************/

#ifndef _OPT_H_
#define _OPT_H_

/* folds constant for a syntax tree  */
void foldConstant( treeNode *t );

#endif