/************************************************************************
 *
 *  gethdr.c, header decoding for tmndecode (H.263 decoder)
 *  Copyright (C) 1995, 1996  Telenor R&D, Norway
 *
 *  Contacts:
 *  Robert Danielsen                  <Robert.Danielsen@nta.no>
 *
 *  Telenor Research and Development  http://www.nta.no/brukere/DVC/
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
 *
 *  Copyright (C) 1997  University of BC, Canada
 *  Modified by: Michael Gallant <mikeg@ee.ubc.ca>
 *               Guy Cote <guyc@ee.ubc.ca>
 *               Berna Erol <bernae@ee.ubc.ca>
 *
 *  Contacts:
 *  Michael Gallant                   <mikeg@ee.ubc.ca>
 *
 *  UBC Image Processing Laboratory   http://www.ee.ubc.ca/image
 *  2356 Main Mall                    tel.: +1 604 822 4051
 *  Vancouver BC Canada V6T1Z4        fax.: +1 604 822 5949
 *
 ************************************************************************/

/* Disclaimer of Warranty
 * 
 * These software programs are available to the user without any license fee
 * or royalty on an "as is" basis. The University of British Columbia
 * disclaims any and all warranties, whether express, implied, or
 * statuary, including any implied warranties or merchantability or of
 * fitness for a particular purpose.  In no event shall the
 * copyright-holder be liable for any incidental, punitive, or
 * consequential damages of any kind whatsoever arising from the use of
 * these programs.
 * 
 * This disclaimer of warranty extends to the user of these programs and
 * user's customers, employees, agents, transferees, successors, and
 * assigns.
 * 
 * The University of British Columbia does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any
 * third-party patents.
 * 
 * Commercial implementations of H.263, including shareware, are subject to
 * royalty fees to patent holders.  Many of these patents are general
 * enough such that they are unavoidable regardless of implementation
 * design.
 * 
 */


/* modified to support annex O true B frames, mikeg@ee.ubc.ca
 * 
 * modified to support general H.263+ syntax, bernae@ee.ubc.ca
 * 
 * based on mpeg2decode, (C) 1994, MPEG Software Simulation Group and
 * mpeg2play, (C) 1994 Stefan Eckart <stefan@lis.e-technik.tu-muenchen.de> */


#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "tmndec.h"
#include "global.h"

/* private prototypes */
static void getpicturehdr _ANSI_ARGS_ ((void));

/* decode headers from one input stream until an End of Sequence or
 * picture start code is found */

int getheader ()
{
  unsigned int code, gob;

  /* look for startcode */
  startcode ();
  code = vic_getbits (PSC_LENGTH);
  gob = vic_getbits (5);
  if (gob == SE_CODE)
    return 0;
  if (gob == 0)
  {
    if (trace)
    {

      fprintf (trace_file, "\nPSC: ");
      printbits ((code << (5 + gob)), 22, 22);
    }
    getpicturehdr ();
    if (syntax_arith_coding)    /* reset decoder after receiving */
      decoder_reset ();         /* fixed length PSC string */
  } 
  else
  {
    if (trace)
    {

      fprintf (trace_file, "\nGBSC: ");
      printbits ((code << (5 + gob)), 22, 22);
    }
  }
  return gob + 1;
}


/* align to start of next startcode */

void startcode ()
{
  /* search for new picture start code */
  while (showbits (PSC_LENGTH) != 1l)
    flushbits (1);
}

/* decode picture header */

static void getpicturehdr ()
{
  int pos, pei, tmp;
  int UFEP = 0;
  int BCI = 0;
  int clock_conversion_code = 0;
  int clock_divisor = 0;
  int extended_temporal_reference = 0;
  int prev_plus_P_temp_ref = 0;

  pos = ld->bitcnt;

  prev_plus_P_temp_ref = temp_ref;
  temp_ref = vic_getbits (8);
  if (trace)
  {
    fprintf (trace_file, "\nTR: ");
    printbits (temp_ref, 8, 8);
  }
  if (trd < 0)
    trd += 256;

  tmp = vic_getbits (1);            /* always "1" */
  if (trace)
    fprintf (trace_file, "\nSpare: %d", tmp);
  if (!tmp)
    if (!quiet)
      printf ("warning: spare in picture header should be \"1\"\n");
  tmp = vic_getbits (1);            /* always "0" */
  if (trace)
    fprintf (trace_file, "\nH.261 distinction bit: %d", tmp);
  if (tmp)
    if (!quiet)
      printf ("warning: H.261 distinction bit should be \"0\"\n");
  tmp = vic_getbits (1);            /* split_screen_indicator */
  if (trace)
    fprintf (trace_file, "\nsplit_screen_indicator: %d", tmp);
  if (tmp)
  {
    if (!quiet)
      printf ("error: split-screen not supported in this version\n");
    exit (-1);
  }
  tmp = vic_getbits (1);            /* document_camera_indicator */
  if (trace)
    fprintf (trace_file, "\ndocument_camera_indicator: %d", tmp);
  if (tmp)
    if (!quiet)
      printf ("warning: document camera indicator not supported in this version\n");

  tmp = vic_getbits (1);            /* freeze_picture_release */
  if (trace)
    fprintf (trace_file, "\nfreeze_picture_release: %d", tmp);
  if (tmp)
    if (!quiet)
      printf ("warning: frozen picture not supported in this version\n");
  tmp = vic_getbits (3);
  if (trace)
  {
    fprintf (trace_file, "\nsource_format: ");
    printbits (tmp, 3, 3);
  }

  if (tmp == EXTENDED_PTYPE)
  {
    if (trace)
      fprintf (trace_file, "\n----------EXTENDED_PTYPE----------");
    plus_type = 1;
    UFEP = vic_getbits (3);
    if (trace)
    {
      fprintf (trace_file, "\nUFEP: ");
      printbits (UFEP, 3, 3);
    }
    if (UFEP == 1)
    {                           /* OPPTYPE */
      if (trace)
        fprintf (trace_file, "\n----------OPTIONAL PLUS PTYPE----------");
      source_format_old = source_format;
      source_format = vic_getbits (3);
      if (trace)
      {
        fprintf (trace_file, "\nsource_format: ");
        printbits (source_format, 3, 3);
      }
    
      /* optional custom picture clock frequency */
      optional_custom_PCF = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\noptional_custom_PCF: ");
        printbits (optional_custom_PCF, 1, 1);
      }
      if (optional_custom_PCF)
      {
        if (!quiet)
          printf ("error: Optional custom picture clock frequency is not supported in this version\n");
        exit (-1);
      }
      mv_outside_frame = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nmv_outside_frame: ");
        printbits (mv_outside_frame, 1, 1);
      }
      long_vectors = (mv_outside_frame ? 1 : 0);
      syntax_arith_coding = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nsyntax_arith_coding: ");
        printbits (syntax_arith_coding, 1, 1);
      }
      adv_pred_mode = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nadv_pred_mode: ");
        printbits (adv_pred_mode, 1, 1);
      }
      mv_outside_frame = (adv_pred_mode ? 1 : mv_outside_frame);
      overlapping_MC = (adv_pred_mode ? 1 : 0);
      use_4mv = (adv_pred_mode ? 1 : 0);
      pb_frame = 0;
      advanced_intra_coding = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nadvanced_intra_coding: ");
        printbits (advanced_intra_coding, 1, 1);
      }
      deblocking_filter_mode = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\ndeblocking_filter_mode: ");
        printbits (deblocking_filter_mode, 1, 1);
      }
      mv_outside_frame = (deblocking_filter_mode ? 1 : mv_outside_frame);
      use_4mv = (deblocking_filter_mode ? 1 : use_4mv);

      slice_structured_mode = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nslice_structured_mode: ");
        printbits (slice_structured_mode, 1, 1);
      }
      if (slice_structured_mode)
      {
        if (!quiet)
          printf ("error: Slice structured mode is not supported in this version\n");
        exit (-1);
      }
      reference_picture_selection_mode = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nreference_picture_selection_mode: ");
        printbits (reference_picture_selection_mode, 1, 1);
      }
#if 0
      if (reference_picture_selection_mode)
      {
        if (!quiet)
          printf ("error: Reference picture selection mode is not supported in this version\n");
        exit (-1);
      }
#endif
      independently_segmented_decoding_mode = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nindependently_segmented_decoding_mode: ");
        printbits (independently_segmented_decoding_mode, 1, 1);
      }
      if (independently_segmented_decoding_mode)
      {
        if (!quiet)
          printf ("error: Independently segmented decoding mode is not supported in this version\n");
        exit (-1);
      }
      alternative_inter_VLC_mode = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nalternative_inter_VLC_mode: ");
        printbits (alternative_inter_VLC_mode, 1, 1);
      }
      modified_quantization_mode = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nmodified_quantization_mode: ");
        printbits (modified_quantization_mode, 1, 1);
      }
      tmp = vic_getbits (4);
      if (trace)
      {
        fprintf (trace_file, "\nspare, reserve, reserve, reserve: ");
        printbits (tmp, 4, 4);
      }
      if (tmp != 8)
      {                         /* OPPTYPE : bit15=1, bit16,bit17,bit18=0 */
        if (!quiet)
          printf ("error: The last 4 bits of OPPTYPE is expected to be 1000\n");
        exit (-1);
      }
    }
    if ((UFEP == 1) || (UFEP == 0))
    {
      if (UFEP == 0)
      {
        if (scalability_mode >= 3)
        {
          horizontal_size = lines[base_source_format];
          vertical_size = pels[base_source_format];

          mb_width = horizontal_size / 16;
          mb_height = vertical_size / 16;

          /* Need to store previous (reference layer) values 
           * for interpolation purposes, as the new values, 
           * i.e. of the spatially scaled layer, depend on 
           * the type of spatial scalability in use. */
          ref_coded_picture_width = coded_picture_width = horizontal_size;
          ref_coded_picture_height = coded_picture_height = vertical_size;
          ref_chrom_width = chrom_width = coded_picture_width >> 1;
          ref_chrom_height = chrom_height = coded_picture_height >> 1;

          source_format = base_source_format;
        }
      }

      /* MMPTYPE */
      if (trace)
        fprintf (trace_file, "\n----------MANDATORY PLUS PTYPE----------");
      pict_type = vic_getbits (3);
      if (trace)
      {
        fprintf (trace_file, "\npict_type: ");
        printbits (pict_type, 3, 3);
      }
      if (pict_type == PCT_IPB)
        pb_frame = IM_PB_FRAMES;
      else
        pb_frame = 0;

      if (PCT_B == pict_type)
      {
        true_B_frame = ON;
        /* Allow motion over picture boundaries, regardless of whether or
         * not UMV is turned on. */
        mv_outside_frame = 1;
        true_b_trb = temp_ref - prev_non_disposable_temp_ref;
      } else
      {
        true_B_frame = OFF;
        prev_non_disposable_temp_ref = next_non_disposable_temp_ref;
        next_non_disposable_temp_ref = temp_ref;
        trd = temp_ref - prev_non_disposable_temp_ref;
      }

      reference_picture_resampling_mode = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nreference_picture_resampling_mode: ");
        printbits (reference_picture_resampling_mode, 1, 1);
      }
      if (reference_picture_resampling_mode)
      {
        if (!quiet)
          printf ("error: Reference picture resampling mode is not supported in this version\n");
        exit (-1);
      }
      reduced_resolution_update_mode = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nreduced_resolution_update_mode: ");
        printbits (reduced_resolution_update_mode, 1, 1);
      }
      if (reduced_resolution_update_mode)
      {
        if (!quiet)
          printf ("error: Reduced resolution update mode is not supported in this version\n");
        exit (-1);
      }
      rtype = vic_getbits (1);      /* rounding type */
      if (trace)
      {
        fprintf (trace_file, "\nrounding_type: ");
        printbits (rtype, 1, 1);
      }
      if (trace)
      {
        fprintf (trace_file, "\nrtype: ");
        printbits (rtype, 1, 1);
      }
      tmp = vic_getbits (3);
      if (trace)
      {
        fprintf (trace_file, "\nreserve, reserve, spare: ");
        printbits (tmp, 3, 3);
      }
      if (tmp != 1)
      {                         /* MPPTYPE : bit7,bit8=0  bit9=1 */
        if (!quiet)
          exit (-1);
      }
    } else
    {
      /* UFEP is neither 001 nor 000 */
      if (!quiet)
        printf ("error: UFEP should be either 001 or 000.\n");
      exit (-1);
    }

    tmp = vic_getbits (1);
    if (trace)
    {
      fprintf (trace_file, "\nCPM: ");
      printbits (tmp, 5, 5);
    }
    if (tmp)
    {
      if (!quiet)
        printf ("error: CPM not supported in this version\n");
      exit (-1);
    }
  
    if (UFEP && (source_format == SF_CUSTOM))
    {
      /* Read custom picture format */
      if (trace)
        fprintf (trace_file, "\ncustom picture format \n");
      CP_PAR_code = vic_getbits (4);
      if (trace)
      {
        fprintf (trace_file, "\nCP_PAR_code: ");
        printbits (CP_PAR_code, 4, 4);
      }
      if (CP_PAR_code != PAR_CIF)
      {
        if (!quiet)
        {
          printf ("error: only 12:11 pixel aspect ratio supported ");
        }
        exit(-1);
      }
      tmp=vic_getbits (9);
      horizontal_size = (tmp + 1 ) * 4;
      if (trace)
      {
        fprintf (trace_file, "\nCP_picture_width_indication: ");
        printbits (tmp, 9, 9);
      }
      tmp = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nspare: ");
        printbits (tmp, 1, 1);
      }
      if (!tmp)
      {
        if (!quiet)
          printf ("error: The 14th bit of Custom Picture Format(CPFMT) should be 1\n");
        exit (-1);
      }
      tmp = vic_getbits (8);
      vertical_size = tmp * 4;
      if (trace)
      {
        fprintf (trace_file, "\nCP_picture_height_indication: ");
        printbits (tmp, 8, 8);
      }
      if ((horizontal_size%16) || (vertical_size%16))
      {
        if(!quiet)
        {
          printf ("error: only factor of 16 custom source format supported\n");
        }
        exit (-1);
      }

      if (CP_PAR_code == EXTENDED_PAR)
      {
        PAR_width = vic_getbits (8);
        PAR_height = vic_getbits (8);
      }
    }

    if (source_format != SF_CUSTOM)
    {
        horizontal_size = lines[source_format];
        vertical_size = pels[source_format];

    }
    
    mb_width = horizontal_size / 16;
    mb_height = vertical_size / 16;

   /* Need to store previous (reference layer) values 
    * for interpolation purposes, as the new values, 
    * i.e. of the spatially scaled layer, depend on 
    * the type of spatial scalability in use. */
    ref_coded_picture_width = coded_picture_width;
    coded_picture_width = horizontal_size;

    ref_coded_picture_height = coded_picture_height;
    coded_picture_height = vertical_size;

    ref_chrom_width = chrom_width; 
    chrom_width = coded_picture_width >> 1;

    ref_chrom_height = chrom_height;
    chrom_height = coded_picture_height >> 1;
    
    if (optional_custom_PCF)
    {
      if (trace)
        fprintf (trace_file, "\noptional_custom_PCF \n");
      if (UFEP)
      {
        clock_conversion_code = vic_getbits (1);
        if (trace)
        {
          fprintf (trace_file, "\nclock_conversion_code: ");
          printbits (clock_conversion_code, 1, 1);
        }
        clock_divisor = vic_getbits (7);
        if (trace)
        {
          fprintf (trace_file, "\nclock_divisor: ");
          printbits (clock_divisor, 7, 7);
        }
        CP_clock_frequency = (int) (1800 / ((float) clock_divisor * (8 + clock_conversion_code)) * 1000);
      }
      /* regardless of the value of UFEP */
      extended_temporal_reference = vic_getbits (2);
      if (trace)
      {
        fprintf (trace_file, "\nextended_temporal_reference: ");
        printbits (extended_temporal_reference, 2, 2);
      }
      temp_ref = extended_temporal_reference << (8 + temp_ref);

      if (PCT_B == pict_type)
      {
        true_b_trb = temp_ref - prev_non_disposable_temp_ref;
      } else
      {
        trd = temp_ref - prev_non_disposable_temp_ref;
      }

      if (trd < 0)
        trd += 1024;
    }
    if (UFEP && slice_structured_mode)
    {
      SSS_rectangular_slices = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nSSS_rectangular_slices: ");
        printbits (SSS_rectangular_slices, 1, 1);
      }
      SSS_arbitary_slice_ordering = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nSSS_arbitary_slice_ordering: ");
        printbits (SSS_arbitary_slice_ordering, 1, 1);
      }
    }

    if ((pict_type == PCT_B) || (pict_type == PCT_EI) || (pict_type == PCT_EP))
    {
      /* optional temporal, SNR and spatial scalability mode in use */
      enhancement_layer_num = vic_getbits (4);
      if (trace)
      {
        fprintf (trace_file, "\nenhancement_layer_num: ");
        printbits (enhancement_layer_num, 4, 4);
      }
      if (UFEP)
      {
        reference_layer_number = vic_getbits (4);
        if (trace)
        {
          fprintf (trace_file, "\nreference_layer_number: ");
          printbits (reference_layer_number, 4, 4);
        }
      }
      if (1 != enhancement_layer_num)
      {
        if (source_format != source_format_old)
        {
          if (source_format != SF_CUSTOM)
          {
            scalability_mode = SPATIAL_SCALABILITY_HV;
          }
          else if (coded_picture_width != ref_coded_picture_width)
          {
            scalability_mode = SPATIAL_SCALABILITY_H;
          }
          else 
          {
            scalability_mode = SPATIAL_SCALABILITY_V;
          }
        }
        else
        {
          scalability_mode = SNR_SCALABILITY;
        }
      }   
    }
    else
    {
      enhancement_layer_num = reference_layer_number = 1;
    }

    if (reference_picture_selection_mode)
    {
      if (UFEP)
      {
        MF_of_reference_picture_selection = vic_getbits (3);
        if (trace)
        {
          fprintf (trace_file, "\nMF_of_reference_picture_selection: ");
          printbits (MF_of_reference_picture_selection, 3, 3);
        }
      }
      TRPI = vic_getbits (1);
      if (trace)
      {
        fprintf (trace_file, "\nTRPI: ");
        printbits (TRPI, 1, 1);
      }
      if (TRPI)
      {
        /* temporal reference for prediction exists */
        temporal_reference_for_prediction = vic_getbits (10);
        if (trace)
        {
          fprintf (trace_file, "\ntemporal_reference_for_prediction: ");
          printbits (temporal_reference_for_prediction, 10, 10);
        }
      }
      /* draft20: 1=yes, 01=no */
      BCI = vic_getbits (1);
      if (!BCI) vic_getbits(1);
      if (trace)
      {
        fprintf (trace_file, "\nBCI: ");
        printbits (BCI, 1, 1);
      }
      if (BCI)
      {
        /* backward channel message exists */
        /* BCM(backward channel message) is not implemented */
        if (!quiet)
          printf ("error: BCM(backward channel message) is not implemented in this version\n");
        exit (-1);
      }
    }
    if (reference_picture_resampling_mode) {
      /* reading RPRP info is not implemented */
      if (!quiet)
        printf ("error: RPRP reading is not implemented in this version\n");
      exit (-1);
    }

    pic_quant = vic_getbits (5);
    quant = pic_quant;

    if (trace)
    {
      fprintf (trace_file, "\nquant: ");
      printbits (quant, 5, 5);
    }
  } 
  else
  {
    plus_type = 0;
    source_format = tmp;
    enhancement_layer_num = reference_layer_number = 1;

    horizontal_size = lines[source_format];
    vertical_size = pels[source_format];

    mb_width = horizontal_size / 16;
    mb_height = vertical_size / 16;

    /* Need to store previous (reference layer) values 
     * for interpolation purposes, as the new values, 
     * i.e. of the spatially scaled layer, depend on 
     * the type of spatial scalability in use. */
    ref_coded_picture_width = coded_picture_width = horizontal_size;
    ref_coded_picture_height = coded_picture_height = vertical_size;
    ref_chrom_width = chrom_width = coded_picture_width >> 1;
    ref_chrom_height = chrom_height = coded_picture_height >> 1;

    trd = temp_ref - prev_plus_P_temp_ref;

    pict_type = vic_getbits (1);
    if (trace)
    {
      fprintf (trace_file, "\npict_type: ");
      printbits (pict_type, 1, 1);
    }
    mv_outside_frame = vic_getbits (1);
    if (trace)
    {
      fprintf (trace_file, "\nmv_outside_frame: ");
      printbits (mv_outside_frame, 1, 1);
    }
    long_vectors = (mv_outside_frame ? 1 : 0);
    syntax_arith_coding = vic_getbits (1);
    if (trace)
    {
      fprintf (trace_file, "\nsyntax_arith_coding: ");
      printbits (syntax_arith_coding, 1, 1);
    }
    adv_pred_mode = vic_getbits (1);
    if (trace)
    {
      fprintf (trace_file, "\nadv_pred_mode: ");
      printbits (adv_pred_mode, 1, 1);
    }
    mv_outside_frame = (adv_pred_mode ? 1 : mv_outside_frame);
    overlapping_MC = (adv_pred_mode ? 1 : 0);
    use_4mv = (adv_pred_mode ? 1 : 0);
    pb_frame = vic_getbits (1);
    if (trace)
    {
      fprintf (trace_file, "\npb_frame: ");
      printbits (pb_frame, 1, 1);
    }
   
    pic_quant = vic_getbits (5);
    quant = pic_quant;

    if (trace)
    {
      fprintf (trace_file, "\nquant: ");
      printbits (quant, 5, 5);
    }

    tmp = vic_getbits (1);
    if (trace)
    {
      fprintf (trace_file, "\nCPM: ");
      printbits (tmp, 5, 5);
    }
    if (tmp)
    {
      if (!quiet)
        printf ("error: CPM not supported in this version\n");
      exit (-1);
    }
  }

  if (pb_frame)
  {
    if (optional_custom_PCF)
    {
      trb = vic_getbits (5);
      if (trace)
      {
        fprintf (trace_file, "\ntrb: ");
        printbits (trb, 5, 5);
      }
    } else
    {
      trb = vic_getbits (3);
      if (trace)
      {
        fprintf (trace_file, "\ntrb: ");
        printbits (trb, 3, 3);
      }
    }

    bquant = vic_getbits (2);
    if (trace)
    {
      fprintf (trace_file, "\nbquant: ");
      printbits (bquant, 2, 2);
    }
  } else
  {
    trb = 0;
  }

#ifdef USE_TIME
  if (framerate > 0 && trd > 0)
    doframerate (0);
#endif

  pei = vic_getbits (1);
  if (trace)
  {
    fprintf (trace_file, "\npei: ");
    printbits (pei, 1, 1);
  }

pspare:
  if (pei)
  {
    /* extra info for possible future backward compatible additions */
    vic_getbits (8);                /* not used */
    pei = vic_getbits (1);
    if (pei)
      goto pspare;              /* keep on reading pspare until pei=0 */
  }
  if (verbose > 0)
  {
    /* $printf("picture header (byte %d)\n",(pos>>3)-4);$ */
    if (verbose > 1)
    {
      printf ("  temp_ref=%d\n", temp_ref);
      /* $printf("  pict_type=%d\n",pict_type); printf("
       * source_format=%d\n", source_format); printf("
       * quant=%d\n",quant); if (syntax_arith_coding) printf("  SAC coding
       * mode used \n"); if (mv_outside_frame) printf("  unrestricted
       * motion vector mode used\n"); if (adv_pred_mode) printf("
       * advanced prediction mode used\n");$ */
      if (pb_frame)
      {
        /* $printf("  pb-frames mode used\n");$ */
        printf ("  trb=%d\n", trb);
        /* $printf("  bquant=%d\n", bquant);$ */
      }
    }
  }
  if (trace)
  {
    fprintf (trace_file, "\n--------------------------------------------------------\n");
  }
}

void initoptions ()
{
  mv_outside_frame = 0;
  syntax_arith_coding = 0;
  adv_pred_mode = 0;
  pb_frame = 0;
  long_vectors = 0;

  plus_type = 0;
  optional_custom_PCF = 0;
  advanced_intra_coding = 0;
  deblocking_filter_mode = 0;
  slice_structured_mode = 0;
  reference_picture_selection_mode = 0;
  independently_segmented_decoding_mode = 0;
  alternative_inter_VLC_mode = 0;
  modified_quantization_mode = 0;
  reduced_resolution_update_mode = 0;
  reference_picture_resampling_mode = 0;
  rtype = 0;
}
