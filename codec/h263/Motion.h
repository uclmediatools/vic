/***********************************************************HeaderBegin*******
 *                                                                         
 * File:	motion.h
 *
 * Author:	K.S.
 * Created:	5-Nov-97
 *                                                                         
 * Description: 
 *
 * Notes: 	
 *
 * Modified:	
 *
 ***********************************************************HeaderEnd*********/


#ifndef _MOTION_H_
#define _MOTION_H_


/***********************************************************CommentBegin******
 *
 * -- MVector -- motion vector structure
 *
 * Author:             K.S.
 *
 * Created:            7-Aug-96
 *	
 * Purpose:            Structure for a motion vector with full- and subpel-
 *                     components and timeshift.
 * 
 * Modified:           -
 *
 *****************************************************************************/
typedef struct {
  short x;     /* horizontal translation */
  short y;     /* verticale translation */
  short sx;    /* horizontal sub-pel translation to the right */
  short sy;    /* verticale sub-pel translation downwards */
  short sF;    /* sub-pel fraction, e.g. 2 for half-pel */
  short t;     /* timeshift */
} MVector;
/***********************************************************CommentEnd********/

MVector *AllocMVector(void); //SV-XXX: FreeBSD

#endif /* _MOTION_STRUCTS_H_ */
