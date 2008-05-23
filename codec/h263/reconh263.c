/************************************************************************
 *
 *  recon.c, motion compensation routines for tmndecode (H.263 decoder)
 *  Copyright (C) 1995, 1996  Telenor R&D, Norway
 *        Karl Olav Lillevold <Karl.Lillevold@nta.no>
 *  
 *  Contacts: 
 *  Karl Olav Lillevold               <Karl.Lillevold@nta.no>, or
 *  Robert Danielsen                  <Robert.Danielsen@nta.no>
 *
 *  Telenor Research and Development  http://www.nta.no/brukere/DVC/
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
 *  
 ************************************************************************/

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any
 * license fee or royalty on an "as is" basis.  Telenor Research and
 * Development disclaims any and all warranties, whether express,
 * implied, or statuary, including any implied warranties or
 * merchantability or of fitness for a particular purpose.  In no
 * event shall the copyright-holder be liable for any incidental,
 * punitive, or consequential damages of any kind whatsoever arising
 * from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs
 * and user's customers, employees, agents, transferees, successors,
 * and assigns.
 *
 * Telenor Research and Development does not represent or warrant that
 * the programs furnished hereunder are free of infringement of any
 * third-party patents.
 *
 * Commercial implementations of H.263, including shareware, are
 * subject to royalty fees to patent holders.  Many of these patents
 * are general enough such that they are unavoidable regardless of
 * implementation design.
 * */


/*
 * based on mpeg2decode, (C) 1994, MPEG Software Simulation Group
 * and mpeg2play, (C) 1994 Stefan Eckart
 *                         <stefan@lis.e-technik.tu-muenchen.de>
 *
 */


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "h263decoder.h"
#include "common.h"
#include "common.p" //SV-XXX: added to avoid implicit declaration of recon_compXXX


void reconstructH263(H263Global *h263Data, int bx, int by, int P, int bdx, int bdy)
{
  int w,h,lx,lx2,dx,dy,xp,yp,comp,sum;
  int x,y,mode,xvec,yvec;
  unsigned char *src[3];

  x = bx/16+1; y = by/16+1;
  lx = h263Data->coded_picture_width;

  if (h263Data->mv_outside_frame) {
    lx2 = h263Data->coded_picture_width + 64;
    src[0] = h263Data->edgeframe[0];
    src[1] = h263Data->edgeframe[1];
    src[2] = h263Data->edgeframe[2];
  }
  else {
    lx2 = h263Data->coded_picture_width;
    src[0] = h263Data->oldrefframe[0];
    src[1] = h263Data->oldrefframe[1];
    src[2] = h263Data->oldrefframe[2];
  }

  mode = h263Data->modemap[y][x];
/*debug_msg("reconstructH263:adv_pred_mode==%d\n",h263Data->adv_pred_mode);*/
  assert(h263Data->adv_pred_mode == 0);

  if (P) {
    /* P prediction */
    if (h263Data->adv_pred_mode) {
      w = 8; h = 8;
      /* Y*/
      for (comp = 0; comp < 4; comp++) {
        xp = bx + ((comp&1)<<3);
        yp = by + ((comp&2)<<2);
        recon_comp_obmc(src[0],h263Data->newframe[0], h263Data->modemap, h263Data->MV,
			h263Data->pb_frame, lx,lx2,comp,w,h,xp,yp);
      }
      if (mode == MODE_INTER4V) {

        sum = h263Data->MV[0][1][y][x]+h263Data->MV[0][2][y][x]+
	  h263Data->MV[0][3][y][x]+h263Data->MV[0][4][y][x];
        dx = SGN(sum)*(vic_roundtab[abs(sum)%16] + (abs(sum)/16)*2);

        sum = h263Data->MV[1][1][y][x]+h263Data->MV[1][2][y][x]+
	  h263Data->MV[1][3][y][x]+h263Data->MV[1][4][y][x];
        dy = SGN(sum)*(vic_roundtab[abs(sum)%16] + (abs(sum)/16)*2);

      }
      else {
        dx = h263Data->MV[0][0][y][x];
        dy = h263Data->MV[1][0][y][x];
        /* chroma rounding */
        dx = ( dx % 4 == 0 ? dx >> 1 : (dx>>1)|1 );
        dy = ( dy % 4 == 0 ? dy >> 1 : (dy>>1)|1 );
      }
      lx>>=1;bx>>=1; lx2>>=1; 
      by>>=1;
      /* Chroma */
      recon_comp(src[1],h263Data->newframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
      recon_comp(src[2],h263Data->newframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
    }
    else { /* normal prediction mode */
      /* P prediction */
      w = 16; h = 16;
      dx = h263Data->MV[0][0][y][x];
      dy = h263Data->MV[1][0][y][x];
      
      /* Y */
      recon_comp(src[0],h263Data->newframe[0], lx,lx2,w,h,bx,by,dx,dy,0);
      
      lx>>=1; w>>=1; bx>>=1; lx2>>=1; 
      h>>=1; by>>=1;  
      /* chroma rounding */
      dx = ( dx % 4 == 0 ? dx >> 1 : (dx>>1)|1 );
      dy = ( dy % 4 == 0 ? dy >> 1 : (dy>>1)|1 );

      /* Chroma */
      recon_comp(src[1],h263Data->newframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
      recon_comp(src[2],h263Data->newframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
    }
  }

  else {
    /* B forward prediction */
    if (h263Data->adv_pred_mode) {
      if (mode == MODE_INTER4V) {
        w = 8; h = 8;
        /* Y*/
        xvec = yvec = 0;
        for (comp = 0; comp < 4; comp++) {
          xvec += dx = (h263Data->trb)*h263Data->MV[0][comp+1][y][x]/h263Data->trd + bdx;
          yvec += dy = (h263Data->trb)*h263Data->MV[1][comp+1][y][x]/h263Data->trd + bdy;
          xp = bx + ((comp&1)<<3);
          yp = by + ((comp&2)<<2);
          recon_comp(src[0],h263Data->bframe[0], lx,lx2,w,h,xp,yp,dx,dy,0);
        }

        /* chroma rounding (table 16/H.263) */
        dx = SGN(xvec)*(vic_roundtab[abs(xvec)%16] + (abs(xvec)/16)*2);
        dy = SGN(yvec)*(vic_roundtab[abs(yvec)%16] + (abs(yvec)/16)*2);

        lx>>=1;bx>>=1; lx2>>=1;
        by>>=1;
        /* Chroma */
        recon_comp(src[1],h263Data->bframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
        recon_comp(src[2],h263Data->bframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
      }
      else {  /* adv_pred_mode but 16x16 vector */
        w = 16; h = 16;

        dx = (h263Data->trb)*h263Data->MV[0][0][y][x]/h263Data->trd + bdx;
        dy = (h263Data->trb)*h263Data->MV[1][0][y][x]/h263Data->trd + bdy;
        /* Y */
        recon_comp(src[0],h263Data->bframe[0], lx,lx2,w,h,bx,by,dx,dy,0);
        
        lx>>=1; w>>=1; bx>>=1; lx2>>=1;
        h>>=1; by>>=1;  

        xvec = 4*dx;
        yvec = 4*dy;

        /* chroma rounding (table 16/H.263) */
        dx = SGN(xvec)*(vic_roundtab[abs(xvec)%16] + (abs(xvec)/16)*2);
        dy = SGN(yvec)*(vic_roundtab[abs(yvec)%16] + (abs(yvec)/16)*2);

        /* Chroma */
        recon_comp(src[1],h263Data->bframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
        recon_comp(src[2],h263Data->bframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
      }
    }
    else { /* normal B forward prediction */

      w = 16; h = 16;
      dx = (h263Data->trb)*h263Data->MV[0][0][y][x]/h263Data->trd + bdx;
      dy = (h263Data->trb)*h263Data->MV[1][0][y][x]/h263Data->trd + bdy;
      /* Y */
      recon_comp(src[0],h263Data->bframe[0], lx,lx2,w,h,bx,by,dx,dy,0);

      lx>>=1; w>>=1; bx>>=1; lx2>>=1;
      h>>=1; by>>=1;  

      xvec = 4*dx;
      yvec = 4*dy;

      /* chroma rounding (table 16/H.263) */ 
      dx = SGN(xvec)*(vic_roundtab[abs(xvec)%16] + (abs(xvec)/16)*2);
      dy = SGN(yvec)*(vic_roundtab[abs(yvec)%16] + (abs(yvec)/16)*2);

      /* Chroma */
      recon_comp(src[1],h263Data->bframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
      recon_comp(src[2],h263Data->bframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
    }
  }
}
