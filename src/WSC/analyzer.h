/****************************************************/
/* File: analyzer.h                                 */
/* Semantic analyzer interface for the WSC compiler */
/****************************************************/

#ifndef _ANALYZER_H_
#define _ANALYZER_H_

/* constructs the symbol table by preorder traversal of the 
 * syntax tree and returns the static size of program
 */
unsigned char *buildSymtab( treeNode *root, byteCodeInfo *info );

/* performs type checking by a postorder syntax 
 * tree traversal
 */
void typeCheck( treeNode *root );

#endif
