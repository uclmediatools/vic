/************************************************************************
 *
 *  main.h, part of tmn (TMN encoder)
 *  Copyright (C) 1997  University of BC, Canada
 *
 *  Contacts: 
 *  Michael Gallant                   <mikeg@ee.ubc.ca>
 *  Guy Cote                          <guyc@ee.ubc.ca>
 *  Berna Erol                        <bernae@ee.ubc.ca>
 *
 *  UBC Image Processing Laboratory   http://www.ee.ubc.ca/image
 *  2356 Main Mall                    tel.: +1 604 822 4051
 *  Vancouver BC Canada V6T1Z4        fax.: +1 604 822 5949
 *
 ************************************************************************/

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any
 * license fee or royalty on an "as is" basis. The University of
 * British Columbia disclaims any and all warranties, whether
 * express, implied, or statuary, including any implied warranties
 * or merchantability or of fitness for a particular purpose.  In no
 * event shall the copyright-holder be liable for any incidental,
 * punitive, or consequential damages of any kind whatsoever arising
 * from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs
 * and user's customers, employees, agents, transferees, successors,
 * and assigns.
 *
 * The University of British Columbia does not represent or warrant
 * that the programs furnished hereunder are free of infringement of
 * any third-party patents.
 *
 * Commercial implementations of H.263, including shareware, are
 * subject to royalty fees to patent holders.  Many of these patents
 * are general enough such that they are unavoidable regardless of
 * implementation design.
 *
*/

  char *seqfilename;
  char *enhance_seqfilename;
  char *streamname;
  char *outputfile;
  char *outputfile_enhance;
  char *diff_filename;
  char *tracefile;

  float mean_frame_rate, ref_frame_rate, frame_rate, seconds;

  int frames,bframes,pframes,Bframes, EIframes, EPframes;
  int wcopies,icopies, write_repeated;
  int total_frames_passed, targetrate;
  int start, end, frame_no, writediff;  
  int QP,QPI;
  int first_frameskip, chosen_frameskip, orig_frameskip, frameskip;
  int mv_outside_frame_old, source_format_old;
  int B_pictures_to_code;
  int prev_I_P_frame_no, prev_pict_type, prev_I_P_pic_TR;
  int arith_used;
  int start_rate_control;


  void InitializeCoder(Pict *pic);
  void ProcessArguments(int argc, char *argv[], Pict *pic);
  void Help();
  void AdvancedHelp();

  FILE *cleared;

  Bits *bits;
  Bits *total_bits;
  Results *total_res;
  Bits *intra_bits;
  Results *intra_res;
  Bits *inter_bits;
  Results *inter_res;
  Bits *b_bits;
  Results *b_res;
  Bits *pb_bits;
  Results *pb_res;
  Bits *scal_bits;
  Results *scal_res;
  Bits *ei_bits;
  Results *ei_res;
  Bits *ep_bits;
  Results *ep_res;

  Results *res;

  int scal_tab[2][9] = { {1,1,1,2,2,1,1,2,2},
                         {1,1,1,1,1,2,2,2,2} };
