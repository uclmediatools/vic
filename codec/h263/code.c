/***********************************************************HeaderBegin*******
 *                                                                         
 * File:        code.c -- Basic routines for prefix-free codes
 *
 * Author:      T.W. M.F.
 *
 * Created:     23-July-96 
 *                                                                         
 * Description: source code of routines for prefix-free codes
 *
 * Notes:       includes code.h
 *
 * Modified:  
 *
 ***********************************************************HeaderEnd*********/


#include <stdlib.h>
#include <string.h> //SV-XXX: added for strlen(), strcpy()

#include "defs.h"
#include "structs.h"

#include "bitOut.h"

#include "Util.h"

#include "code.h"
#include "Motion.h"
#include "common.p"


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- AllocCode -- Allocate Code structure
 *
 * Author:           M.F.
 *
 * Created:          23-July-96		
 *	
 * Purpose:          Allocates the Code structure.
 *
 * Arguments in:     int size    : Number of codewords in Code.
 *                   int eventDim: Dimension of event array
 *                   char *title :
 *
 * Arguments in/out: none
 *
 * Arguments out:    none
 *
 * Return values:    Code        : A pointer to an allocated Code. 
 *                                 If the Code could not be allocated,
 *                                 a NULL pointer is returned.
 *
 * Side effects:     none
 *
 * Description:      Allocates structure, Codeword table, and 
 *                   root node of code tree.
 *
 * See also:         FreeCode, LoadCode, GenerateHuffmanCode
 *	
 * Modified:         T.W., 15-Nov-1996, Adapted towards CIDS library.
 *
 ****************************************************************************/
Code *AllocCode(int size, int eventDim, char *title)
/***********************************************************CommentEnd********/
{
  Code *code = (Code *)malloc(1 * sizeof(Code));
  if(!code) return(NULL);

  code->size = size;                /* Number of codewords */
  code->eventDim = eventDim;        /* Dimension of event array */
  code->title = (char *)malloc((strlen(title) + 1) * sizeof(char));
  strcpy(code->title, title);      /* Name of Code */

  /* Allocate table of Codewords */
  code->cwTable = (Codeword *)calloc(size, sizeof(Codeword));
  if(!code->cwTable){
    free(code); 
    code = 0;
    return(NULL);
  }

  /* Allocate event array to each codeword */
  code->event = (int **)calloc(size, sizeof(int *));
  if(eventDim>=1) {
    int i;
    for (i=0;i<size;++i) {
      code->event[i] = (int *)calloc(eventDim, sizeof(int));
    }
  }

  /* Alloc code tree */
  code->codeTree = AllocCodeTree();
  if(!code->codeTree){
    free(code->cwTable); 
    code->cwTable = 0;
    free(code); 
    code = 0;
    return(NULL);
  }

  return(code);
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- InitCode -- Init Code structure
 *
 * Author:            K.S.
 *	
 * Purpose:           Inits code structure (including AllocCode).
 *                    Can be used instead reading from file with 'LoadCode'.
 *
 *
 *	              The code must be prefix-free. If not, a NULL pointer 
 *	              is returned instead of the pointer to Code.
 *	               
 * Arguments in:      char  *title                 Name of code table
 *                                                 (not important).
 *                    int   size                   size of code table
 *                    int   eventDim               event dimension
 *                    int   event[size * eventDim] field containing the events.
 *                    char  *cw[size]              field containing the
 *                                                 codewords as strings.
 *
 * Arguments in/out:  none
 *
 * Arguments out:     none
 *
 * Return values:     Code     : A pointer to an allocated and initialized
 *                               Code. If the Code is
 *                               not correct, a NULL pointer is returned.
 *	
 * Side effects:      none
 *	
 * Example:           In order to init a Code 'code', use
 *                      int  event[2 * 3] = {0, 1, 2,
 *                                           10, 35, 94};
 *                      char *cw[2]       = {"01001"; "10010"};
 *                      Code *code        = InitCode("Kaese", 2, 3, event, cw);
 *
 * Description:       
 *                    The code is allocated and the code tree is generated.
 *                    * The title must consist of one string without a space
 *                      and max. 79 characters.
 *
 * See also:          LoadCode, SaveCode, GenerateHuffmanCode, AllocCode
 *	
 * Modified:          
 *
 ***************************************************************************/
Code *InitCode(char *title, int size, int eventDim, int *event, char *cw[]) 
/**********************************************************CommentEnd*******/
{
  int i, j;
  Code *code;
  
  code = AllocCode(size, eventDim, title);

  /* Read events and corresponding codewords */
  for (i=0;i<size;++i) {
    for (j=0;j<eventDim;++j) {
      code->event[i][j] = event[i * eventDim + j];
    }
    StringToCodeword(cw[i], &(code->cwTable[i]));
  }

  /* Generate code tree and check whether the code is prefix-free */
  if(!GenerateCodeTree(code)) {
    fprintf(stderr, "InitCode: Wrong code with title %s\n", 
	    title);
    return (NULL);
  } else {    
    return (code);
  }
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- GenerateCodeTree -- Generate code tree using existing code table
 *
 * Author:            M.F.
 *	
 * Purpose:           Generates code tree for prefix-free code decoding.
 *	
 * Arguments in:      none
 *
 * Arguments in/out:  Code *code: Code in which the event is searched.
 *
 * Arguments out:     none
 *
 * Return values:     1: if the code is prefix-free 
 *                    0: otherwise
 *	
 * Side effects:      none
 *	
 *
 * See also:          AddCodewordToCodeTree
 *	
 * Modified:          
 *
 ***************************************************************************/
int GenerateCodeTree(Code *code)
/**********************************************************CommentEnd*******/
{
  int i;
  
  /* generate code tree */
  for(i=0; i<code->size; i++){
    if(code->cwTable[i].size){
      if(!AddCodewordToCodeTree(&(code->cwTable[i]), i, 0, code->codeTree)){
	return(0);
      }
    }
  }
  return(1);
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- AllocCodeTree -- Allocates BinTree-Node and initializes it
 *
 * Author:            M.F.
 *	
 * Purpose:           Allocate BinTree-Node and initialize it.
 *	
 * Arguments in:      none
 *
 * Arguments in/out:  none
 *
 * Arguments out:     none
 *
 * Return values:     Allocated BinTreeNode 
 *
 * Side effects:      none
 *
 * See also:          AddCodewordToCodeTree, GenerateCodeTree
 *
 * Modified:          
 *
 ***************************************************************************/
BinTreeNode *AllocCodeTree(void)
/**********************************************************CommentEnd*******/
{
  /* allocat new root node for code tree */
  BinTreeNode *btn = AllocBinTreeNode();
  /* mark new node unused */
  btn->e.i = -2;
  return(btn);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- StringToCodeword -- Convert string to codeword
 *
 * Author:           M.F.
 *
 * Created:          23-July-96		
 *	
 * Purpose:          Converts string to codeword.
 *
 * Arguments in:     char *str   : result
 *
 * Arguments in/out: none
 *
 * Arguments out:    Codeword *cw: contains codeword
 *
 * Return values:    none
 *
 * Side effects:     none
 *
 * See also:         CodewordToString
 *
 * Modified:
 *
 *****************************************************************************/
int StringToCodeword(char *str, Codeword *cw)
/***********************************************************CommentEnd********/
{
  int i;
  int len = strlen(str);


  if(len>MAX_CODEWORD_LENGTH){
    /* codeword too long ! */
    return(0);
  } else{
    cw->size = len;
    cw->bits = 0;
    /* generate codeword */
    for(i=0; i<len; i++){
      if(str[i] == '1') cw->bits |= 1<<(len-1-i);
    }
    return(1);
  }
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- PutIndexToBitstr -- Write  codeword to bitstream 
 *
 * Author:           M.F.
 *
 * Created:          23-July-96		
 *	
 * Purpose:          Writes the codeword specified by 'index' in 
 *                   Code 'code' to the Bitstr 'b'.
 *
 * Arguments in:     Code *code: HuffmaCode in which the codeword
 *                               is contained if 'index' is valid.
 *                   int index : Index indicating the specified codeword.
 *
 * Arguments in/out: Bitstr *b : Bitstream to which the codeword is added. 
 *
 * Arguments out:    none
 *
 * Return values:    1: success, if index exists
 *		     0: failure
 *
 * Side effects:     The buffer fullness of Bitstr 'b' is increased 
 *                   accordingly.
 *
 * Description:      Table-look-up with index.
 *                   Uses PutBit to write to bitstream.
 *
 * Example:          Bitstr *b = AllocBitstr(1024);
 *                   PutIndexToBitstr(code, index, b);
 * 
 * See also:         GetIndexFromBitstr, PutBit
 *	
 * Modified:         T.W., 15-Nov-1996, Adapted towards CIDS library.
 *	
 ****************************************************************************/
int PutIndexToBitstr(Code *code, int index, Bitstr *b)
/***********************************************************CommentEnd********/
{
  /*int i;*/
  Codeword cw;
  
  if( (index>=0) && (index<code->size)){
    cw = code->cwTable[index];
    PutNumberX(cw.bits, cw.size, b);
    /*    for (i=0; i<cw.size; i++){
      if ( cw.bits & ( 1<<(cw.size-i-1)) ){
	PutBitX(1, b);
      } else { 
	PutBitX(0, b);
      }
    }*/
    return(1);
  } else{
    return(0);
  }
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- GetIndexFromEvent -- Read an event from Code given an index
 *
 * Author:            T.W.
 *	
 * Purpose:           Reads an event from Code given an index.
 *                    If the event is not part of Code, a '0'
 *                    is returned otherwise '1'. 
 *	
 * Arguments in:      Code *code: Code in which the event is searched.
 *                    int *event: Event which's index is to be found.
 *                                Attention: the array 'event' is assumed
 *                                to be of size code->eventDim.
 *
 * Arguments in/out:  none             
 *
 * Arguments out:     int *index: Corresponding index if the event is part
 *                                of Code otherwise it is left unchanged.
 *
 * Return values:     1: success, the event is found
 *                    0: failure, the event is NOT found
 *	
 * Side effects:      none
 *	
 * See also:          GetEventFromIndex
 *	
 * Example:           GetIndexFromEvent(code, event, &index);
 * 
 * Description:       Searches the entire event space element by element.
 *                    When the first occurence is found the function terminates
 *                    with success.
 *
 * Modified:          
 *
 ***************************************************************************/
int GetIndexFromEvent(Code *code, int *event, int *index)
/**********************************************************CommentEnd*******/
{
  int i = 0;
  int j, identityCount;


  while (i<code->size) {
    identityCount = 0;
    for (j=0;j<code->eventDim;++j) {
      identityCount += (code->event[i][j]==event[j]);
    }
    if (identityCount==code->eventDim) {
      *index = i;
      return(1);
    } else {
      ++i;
    }
  }
  return(0);
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- AddCodewordToCodeTree -- Add codeword to existing code tree
 *
 * Author:            M.F.
 *	
 * Purpose:           Add codeword to existing code tree.
 *                    Checks whether code is prefix-free.
 *	
 * Arguments in:      Codeword *cw: codeword to be marked in 'code->codeTree'
 *                    int index   : associated index to 'cw'
 *                    int level   : current depth in tree
 *
 * Arguments in/out:  BinTreeNode *code->codeTree: 
 *                                 BinTree in which the codeword is marked.
 *
 * Arguments out:     none
 *
 * Return values:     1: if the code is prefix-free 
 *                    0: otherwise
 *	
 * Side effects:      none
 *	
 * See also:          GenerateCodeTree
 *	
 * Modified:          
 *
 ***************************************************************************/
int AddCodewordToCodeTree(Codeword *cw, int index, int level, BinTreeNode *btn)
/**********************************************************CommentEnd*******/
{
  /* 'AddCodeCodeword' adds the necessary tree structure and 'index' */
  /* to the leaf node. (recursive function! start with root!)*/
  if(level<cw->size){
    /* codeword incomplete */
    if(btn->e.i < 0){
      /* extend only unused or interior nodes! */
      if( !(cw->bits & ( 1<<(cw->size-1-level)) ) ){
	/* add node for zero branch if necessary */
	if(!btn->btn[0]){
	  btn->e.i = -1; /* mark node interior */
	  btn->btn[0] = AllocCodeTree();
	  btn->ndType[0] = 1;
	}
	/* continue recursively */
	if(!AddCodewordToCodeTree(cw, index, level+1, btn->btn[0])) 
	  return(0);
      } else{
	/* add node for one branch if necessary */
	if(!btn->btn[1]){
	  btn->e.i = -1; /* mark node interior */
	  btn->btn[1] = AllocCodeTree();
	  btn->ndType[1] = 1;
	}
	/* continue recursively */
	if(!AddCodewordToCodeTree(cw, index, level+1, btn->btn[1]))
	  return(0);
      }
    } else{
      /* can not extend used leaf node! ERROR! */
      return(0);
    }
  } else{
    /* codeword complete */
    if(btn->e.i > -2){
      /* interior of the tree or used leaf node! ERROR! */
      return(0);
    } else{
      /* mark leaf node with codeword index */
      btn->e.i = index;
    }
  }
  /* correct codeword obtained */
  return(1);
}


/***********************************************************CommentBegin******
 *
 * -- AllocBinTreeNode -- Allocate BinTreeNode
 *
 * Author : M.F., T.W.
 *
 * Created : 17-July-96
 *
 * Purpose : creates a new node and marks it as leaf node
 * 
 * Prototype : BinTreeNode *AllocBinTreeNode(void);
 *          
 * Variables :
 *           
 * Side effects :
 *
 * See also : FreeBinTreeNode
 *
 * Modified :
 *
 ***********************************************************CommentEnd********/
BinTreeNode *AllocBinTreeNode(void){
  BinTreeNode *btn = (BinTreeNode *)calloc(1, sizeof(BinTreeNode));
  return(btn);
}
