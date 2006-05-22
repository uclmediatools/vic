/***********************************************************HeaderBegin*******
 *                                                                         
 * File:        code.h
 *
 * Author:      K.S.
 * Created:     5-Nov-97
 *                                                                         
 * Description: Definition of routines for codes
 *
 * Modified:  
 *
 ***********************************************************HeaderEnd*********/

#ifndef _CODE_H
#define _CODE_H


#define MAX_CODEWORD_LENGTH 32
#define MAX_SYMS 16380

/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- Codeword -- Codeword Type
 *
 * Author:       M.F.
 *
 * Created:      23-July-96
 *
 * Description:  int size : length of codeword
 *               int cw   : codeword register
 *
 * Example:      size = 3
 *                           ------------------
 *                           ..| x | 0 | 1 | 1 |
 *                           ------------------
 *                 significance  8   4   2   1
 * 
 *               codeword: "011"
 *
 *
 * Note:         maximum codeword length is 32 bits:
 *               #define MAX_CODEWORD_LENGTH 32
 *
 * See also:     -
 *
 * Modified:     T.W., 15-Nov-1996, Adapted towards CIDS library
 *
 *****************************************************************************/
typedef struct{
  int size;
  int bits;
} Codeword;
/***********************************************************CommentEnd********/


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- Element -- Element 
 *
 * Author:       M.F., T.W.
 *
 * Created:      17-July-96
 *
 * Description:  generic element which contains the basic types
 *               int           i;
 *               unsigned int  ui;
 *               float         f;
 *               void          *p;
 *  
 * Note:         -
 *
 * See also:     -
 *
 * Modified:     -
 *
 *****************************************************************************/
typedef union {
  int           i;
  unsigned int  ui;
  float         f;
  void          *p;
} Element;
/***********************************************************CommentEnd********/
 
 
/***********************************************************CommentBegin******
 *
 * -- BinTreeNode -- Binary Tree Node Type
 *
 * Author:      M.F., T.W.
 *
 * Created:     17-July-96
 *
 * Description: e:        node data
 *              ndType:   type of node
 * 
 *              ndType[branch] = 0/1
 *
 *              example:  branch
 *                        0   1
 *                       --- --- 
 *                      | 1 | 0 |
 *                       --- ---
 *              meaning:
 *              0 : NO subtree for the one (right) branch
 *              1 : subtree for the zero (left) branch 
 *	            (root node is referenced by btn[0])
 *              btn[0/1]: points to the child node connected by the zero/one
 *                        branch.
 *
 * Note:        Use recursive algorithms! (e.g. BinTreeFunctional)
 *
 * See also:    -
 *
 * Modified:
 *
 ***********************************************************CommentEnd********/
typedef struct btn_struct {
  Element           e;         
  unsigned char     ndType[2];  
  struct btn_struct *btn[2];    
} BinTreeNode;


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- Code -- Code struct type definition
 *
 * Author:       M.F.
 *
 * Created:      23-July-96
 *
 * Description:  int size               : number of codewords
 *               Codeword *cwTable      : table of codewords
 *               Int eventDim           : size of event array      
 *               Int **event            : event array associated with each 
 *                                        codeword
 *               BintTreeNode *codeTree : code tree with codeword
 *                                        indexes at the leaf nodes
 *
 *               The code tree consists of 'BinTreeNode's.
 *               'e.i' holds the index of each leaf node (codeword)
 *               If the node is not a leaf, 'e.i' is set to -1.
 *               If the node is unused, 'e.i' is set to -2.
 *               btn[0/1] points to the child node connected by the zero/one
 *               branch.
 *
 * Note:         -
 *
 * See also:     Codeword, BinTreeNode 
 *
 * Modified:     T.W., 15-Nov-1996, Adapted towards CIDS library
 *
 *****************************************************************************/
typedef struct{
  int         size;
  char        *title;
  Codeword    *cwTable;
  int         eventDim;
  int         **event;
  /* private */
  BinTreeNode *codeTree;
} Code;

Code *InitCode(char *title, int size, int eventDim, int *event, char *cw[]); //SV-XXX: FreeBSD

/***********************************************************CommentEnd********/


#endif /* _CODE_H  */
