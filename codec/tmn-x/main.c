/************************************************************************
 *
 *  main.c, main module of tmn (TMN encoder).
 *  tmn is an H.263+ (H.263 ver. 2.0) encoder somewhat based on the 
 *  Test Model Near-term (TMN8) in the ITU-T LBC Experts Group.
 *
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

#include "h263coder.h"
#include "main.h"

//SV-XXX: defined UNUSED() macro for unused variables
#ifndef UNUSED
#define UNUSED(x) (x) = (x)
#endif


FILE           *streamfile;

struct ENCODER_STATE *
h263_init_encoder_1()
{
    struct ENCODER_STATE *s;

    s = malloc(sizeof(struct ENCODER_STATE));
    memset(s, 0, sizeof(struct ENCODER_STATE));

    s->pic = (Pict *) malloc(sizeof(Pict));

    bits = (Bits *) calloc(1, sizeof(Bits));
    total_bits = (Bits *) calloc(1, sizeof(Bits));
    total_res = (Results *) calloc(1, sizeof(Results));
    intra_bits = (Bits *) calloc(1, sizeof(Bits));
    intra_res = (Results *) calloc(1, sizeof(Results));
    inter_bits = (Bits *) calloc(1, sizeof(Bits));
    inter_res = (Results *) calloc(1, sizeof(Results));
    scal_bits = (Bits *) calloc(1, sizeof(Bits));
    scal_res = (Results *) calloc(1, sizeof(Results));
    ei_bits = (Bits *) calloc(1, sizeof(Bits));
    ei_res = (Results *) calloc(1, sizeof(Results));
    ep_bits = (Bits *) calloc(1, sizeof(Bits));
    ep_res = (Results *) calloc(1, sizeof(Results));

    res = (Results *) calloc(1, sizeof(Results));

#ifndef FASTIDCT
    sed_init_sed_idctref();
#endif

    InitializeCoder(s->pic);
    return s;
}

void
h263_init_encoder_2(struct ENCODER_STATE *s)
{
    /* rate control variables */
    if (targetrate == 0) {
	rate_control_method = NO;
    } else {
	s->pic->bit_rate = targetrate;
	InitializeRateControl();
    }

    /* pic->src_frame_rate needed to update UFEP */
    s->pic->src_frame_rate = (int) (ref_frame_rate / orig_frameskip);
    s->DelayBetweenFramesInSeconds = (float) 1.0 / (float) s->pic->src_frame_rate;

    frame_rate = ref_frame_rate / (float) (orig_frameskip * chosen_frameskip);

    if (rate_control_method == OFFLINE_RC)
	fprintf(stdout, "Encoding frame rate  : %.2f\n", frame_rate);
    else
	fprintf(stdout, "Encoding frame rate  : variable\n");

    fprintf(stdout, "Reference frame rate : %.2f\n", ref_frame_rate);
    fprintf(stdout, "Orig. seq. frame rate: %.2f\n\n",
	    ref_frame_rate / (float) orig_frameskip);

    /* check for mode restrictions */
    if (syntax_arith_coding && (alternative_inter_vlc || modified_quantization)) {
	fprintf(stderr, "syntax_arith_coding cannot be used with alternative_inter_vlc or  modified_quantization options\n");
	exit(-1);
    }
    /* Initialize bitcounters */
    sed_initbits();

    /* Needed for Annex O temporal scalability */
    s->fixed_frameskip = chosen_frameskip * orig_frameskip;
    s->True_B_frameskip = s->fixed_frameskip;

    /* number of seconds to encode */
    seconds = (end - start + chosen_frameskip) * orig_frameskip / ref_frame_rate;

    if (trace) {
	strcpy(tracefile, "trace.enc");
	/* Open trace-file for writing */
	if ((tf = fopen(tracefile, "w")) == NULL) {
	    fprintf(stderr, "Unable to open tracefile\n");
	    exit(-1);
	}
    }
    if (pb_frames || improved_pb_frames) {
	pb_bits = (Bits *) calloc(1, sizeof(Bits));
	pb_res = (Results *) calloc(1, sizeof(Results));
    }
    if (successive_B_frames) {
	b_bits = (Bits *) calloc(1, sizeof(Bits));
	b_res = (Results *) calloc(1, sizeof(Results));
    }
    if (scalability_mode) {
	scal_bits = (Bits *) calloc(1, sizeof(Bits));
	scal_res = (Results *) calloc(1, sizeof(Results));
    }
    s->pic->UFEP = 0;

}

int
h263_encode_one_frame(struct ENCODER_STATE *s, int i, int tr)
{
    DeterminePictureType(&frame_no, s->pic, s->P_frameskip, s->True_B_frameskip, i);

#ifdef VIC
    s->pic->TR = tr;
#endif
#ifdef MALLOC_DEBUG_ON
    fprintf(stderr,"malloc_count: %d\n",malloc_count);
#endif

    if (-1 == s->pic->picture_coding_type) {
	return -1;
    }
    switch (s->pic->picture_coding_type) {

    case PCT_INTRA:

	sed_pels = base_sed_pels;
	sed_lines = base_sed_lines;
	csed_pels = base_sed_pels / 2;

	s->image = ReadImage(seqfilename, frame_no, headerlength);
	s->curr_image = FillImage(s->image);
	fprintf(stdout, "Coding I frame... ");

	s->pic->QUANT = QPI;
	s->pic->DQUANT = 0;

	s->curr_recon = CodeOneIntra(s->curr_image, QPI, bits, s->pic);

	CalculateStatistics(s->curr_image, s->curr_recon, NULL, NULL, bits, QPI, s->pic);

	/* Special handling if temporal scalability. B frames will be 
	 * inserted between the first I-P pair. So the future P frame will
	 * be the next frame coded. It sets prev_P_image and prev_P_recon
	 * based on to the previous next_P_image and next_P_recon, 
	 * respectively. */
	if (successive_B_frames) {

	    /* Free the past prev_P_image and prev_P_recon, no longer needed */
	    if (NULL != s->prev_P_image) {
		FreeImage(s->prev_P_image);
		FreeImage(s->prev_P_recon);
	    }
	    /* Set new prev_P_image and prev_P_recon to old next_P_image and 
	     * next_P_recon respectively */
	    s->prev_P_image = s->next_P_image;
	    s->prev_P_recon = s->next_P_recon;

	    /* For future I_P pictures, curr_image and curr_recon are set to 
	     * prev_image and prev_recon. For B picture prediction, curr_image
	     * and curr_recon represent next_P_image and next_P_recon. */
	    s->prev_image = s->next_P_image = s->curr_image;
	    s->prev_recon = s->next_P_recon = s->curr_recon;
	} else {
	    if (NULL != s->prev_image) {
		FreeImage(s->prev_image);
		FreeImage(s->prev_recon);
	    }
	    s->prev_image = s->curr_image;
	    s->prev_recon = s->curr_recon;
	}

	s->prev_I_P_quant = s->pic->QUANT;

	/* Frame layer Rate Control */
	frameskip = FrameLayerRC(s->pic);

	fprintf(stdout, "Finished INTRA\n");

	break;

    case PCT_INTER:

	pframes++;
	frames++;

	sed_pels = base_sed_pels;
	sed_lines = base_sed_lines;
	csed_pels = base_sed_pels / 2;

	if (rate_control_method == OFFLINE_RC) {
	    s->stored_image = s->curr_image;
	} else {
	    s->image = ReadImage(seqfilename, frame_no, headerlength);
	    s->curr_image = FillImage(s->image);
	}

	fprintf(stdout, "Coding P frame %d... ", frame_no);
	fflush(stdout);

	/* Set QP to pic->QUANT from previous encoded picture */
	QP = s->prev_I_P_quant;
	s->pic->DQUANT = 0;

	s->curr_recon = InitImage(sed_pels * sed_lines);

	CodeOneOrTwo(s->curr_image, NULL, s->prev_image, s->prev_recon,
		     QP, s->P_frameskip * orig_frameskip, bits, s->pic,
		     NULL, s->curr_recon);

	CalculateStatistics(s->curr_image, s->curr_recon, NULL, NULL, bits, QP, s->pic);

	/* Specal handling for case of temporal scalability. The P frame being coded
	 * is the future P frame. */
	if (successive_B_frames) {
	    /* Free the past prev_P_image and prev_P_recon, no longer needed */
	    if (NULL != s->prev_P_image) {
		FreeImage(s->prev_P_image);
		FreeImage(s->prev_P_recon);
	    }
	    /* Set new prev_P_image and prev_P_recon to old next_P_image and 
	     * next_P_recon respectively */
	    s->prev_P_image = s->next_P_image;
	    s->prev_P_recon = s->next_P_recon;

	    /* For future I_P pictures, curr_image and curr_recon are set to 
	     * prev_image and prev_recon. For B picture prediction, curr_image
	     * and curr_recon represent next_P_image and next_P_recon. */
	    s->prev_image = s->next_P_image = s->curr_image;
	    s->prev_recon = s->next_P_recon = s->curr_recon;
	} else {
	    if (NULL != s->prev_recon) {
		FreeImage(s->prev_recon);
		if ((!scalability_mode) ||
		    (scalability_mode && (s->prev_image != s->prev_enhancement_image))) {
		    FreeImage(s->prev_image);
		}
	    }
	    s->prev_image = s->curr_image;
	    s->prev_recon = s->curr_recon;
	}

	/* Frame Layer Rate Control */
	frameskip = FrameLayerRC(s->pic);
	s->prev_I_P_quant = s->pic->QUANT;

	break;

    case PCT_PB:
    case PCT_IPB:

	bframes++;
	pframes++;
	frames += 2;

	sed_pels = base_sed_pels;
	sed_lines = base_sed_lines;
	csed_pels = base_sed_pels / 2;

	/* Set QP to pic->QUANT from previous encoded picture */
	QP = s->prev_I_P_quant;
	s->pic->DQUANT = 0;

	s->image = ReadImage(seqfilename, frame_no, headerlength);
	s->curr_image = FillImage(s->image);
	s->B_image = s->curr_image;

	s->image = ReadImage(seqfilename, frame_no + s->P_frameskip, headerlength);
	s->curr_image = FillImage(s->image);

	s->pic->TRB = s->P_frameskip * orig_frameskip;
	frame_no += s->P_frameskip;

	/* Code two frames as a PB-frame */
	s->B_recon = InitImage(sed_pels * sed_lines);
	fprintf(stdout, "Coding PB frames %d and %d... ",
		frame_no - s->P_frameskip, frame_no);
	fflush(stdout);
	s->curr_recon = InitImage(sed_pels * sed_lines);

	/* Code PP or PB pictures. */
	CodeOneOrTwo(s->curr_image, s->B_image, s->prev_image, s->prev_recon,
		     QP, 2 * s->P_frameskip * orig_frameskip, bits, s->pic,
		     s->B_recon, s->curr_recon);

	s->prev_image = s->curr_image;
	s->prev_recon = s->curr_recon;

	CalculateStatistics(s->curr_image, s->curr_recon, s->B_image, s->B_recon, bits, s->pic->BQUANT, s->pic);

	/* Frame Layer Rate Control */
	frameskip = FrameLayerRC(s->pic);

	s->prev_I_P_quant = s->pic->QUANT;

	break;

    case PCT_B:

	Bframes++;
	frames++;

	sed_pels = base_sed_pels;
	sed_lines = base_sed_lines;
	csed_pels = base_sed_pels / 2;

	s->image = ReadImage(seqfilename, frame_no, headerlength);
	s->curr_image = FillImage(s->image);

	fprintf(stdout, "Coding B frame... %d ", frame_no); //SV-XXX: malformed format string, missing %d
	fflush(stdout);

	/* Set QP to b_picture_quant, no rate control for true B yet */
	QP = b_picture_quant;
	s->pic->DQUANT = 0;

	/* Code the next frame as true-B. */
	s->curr_recon = InitImage(sed_pels * sed_lines);

	mv_outside_frame_old = mv_outside_frame;
	mv_outside_frame = ON;

	/* Code the current B picture. */
	CodeOneTrueB(s->next_P_image, s->curr_image, s->prev_P_image, s->prev_P_recon,
		     QP, s->True_B_frameskip * orig_frameskip, bits, s->pic,
		     s->curr_recon, s->next_P_recon);

	CalculateStatistics(s->curr_image, s->curr_recon, NULL, NULL, bits, QP, s->pic);

	mv_outside_frame = mv_outside_frame_old;

	break;

    case PCT_EI:

	EIframes++;

	sed_pels = enhancement_sed_pels;
	sed_lines = enhancement_sed_lines;
	csed_pels = enhancement_sed_pels / 2;

	fprintf(stdout, "Coding EI frame %d... ", frame_no);
	fflush(stdout);

	/* QP fixed for enhancement layer */
	QP = enhancement_layer_quant;
	s->pic->DQUANT = 0;

	s->pic->UFEP = 1;
	s->pic->RLNUM = BASE_LAYER_NUM;
	s->pic->ELNUM = ENHANCEMENT_LAYER_NUM;

	/* For all possible spatial scalings */
	if (scalability_mode >= 3) {
	    /* No MVs from reference layer, so don't need to frame recon */
	    s->curr_reference_recon = sed_UpsampleReferenceLayerPicture(s->prev_recon);

	    /* Enhancement layer image is temporally simultaneous, same image as was
	     * just coded in reference layer, i.e. stored as prev_image */
	    if (enhance_seqfilename[0] == '\0') {
		s->curr_image = sed_UpsampleReferenceLayerPicture(s->prev_image);
	    } else {
		s->image = ReadImage(enhance_seqfilename, frame_no, headerlength);
		s->curr_image = FillImage(s->image);
	    }

	    source_format_old = s->pic->source_format;
	    s->pic->source_format = (scalability_mode >= 7) ? source_format_old + 1 : SF_CUSTOM;
	} else {
	    s->curr_reference_recon = s->prev_recon;
	    s->curr_image = s->prev_image;
	}

	mv_outside_frame_old = mv_outside_frame;
	mv_outside_frame = ON;

	s->curr_enhancement_recon = InitImage(sed_pels * sed_lines);

	CodeOneEI(s->curr_image, s->curr_reference_recon, QP, bits, s->pic, s->curr_enhancement_recon);

	/* Must apply DF here, as only now do we have fully reconstructed, enhanced picture */
	if (deblocking_filter) {
	    EdgeFilter(s->curr_enhancement_recon, s->pic);
	}
	CalculateStatistics(s->curr_image, s->curr_enhancement_recon, NULL, NULL, bits, QP, s->pic);

	mv_outside_frame = mv_outside_frame_old;

	/* Free the past prev_enhancement_image and prev_enhancement_recon, 
	 * no longer needed */
	if (NULL != s->prev_enhancement_image) {
	    FreeImage(s->prev_enhancement_image);
	    FreeImage(s->prev_enhancement_recon);
	}
	/* Set new prev_enhancement_image and prev_enhancement_recon to old 
	 * next_P_image and  next_P_recon respectively */
	s->prev_enhancement_image = s->curr_image;
	s->prev_enhancement_recon = s->curr_enhancement_recon;

	if (scalability_mode >= 3) {
	    s->pic->source_format = source_format_old;
	}
	break;

    case PCT_EP:

	EPframes++;

	sed_pels = enhancement_sed_pels;
	sed_lines = enhancement_sed_lines;
	csed_pels = enhancement_sed_pels / 2;

	fprintf(stdout, "Coding EP frame %d... ", frame_no);
	fflush(stdout);

	/* QP fixed for enhancement layer */
	QP = enhancement_layer_quant;
	s->pic->DQUANT = 0;

	s->pic->UFEP = 1;
	s->pic->RLNUM = BASE_LAYER_NUM;
	s->pic->ELNUM = ENHANCEMENT_LAYER_NUM;

	/* For all possible spatial scalings */
	if (scalability_mode >= 3) {
	    /* No MVs from reference layer, so don't need to frame recon */
	    s->curr_reference_recon = sed_UpsampleReferenceLayerPicture(s->prev_recon);

	    /* Enhancement layer image is temporally simultaneous, same image as was
	     * just coded in reference layer, i.e. stored as prev_image */
	    if (enhance_seqfilename[0] == '\0') {
		s->curr_image = sed_UpsampleReferenceLayerPicture(s->prev_image);
	    } else {
		s->image = ReadImage(enhance_seqfilename, frame_no, headerlength);
		s->curr_image = FillImage(s->image);
	    }

	    source_format_old = s->pic->source_format;
	    s->pic->source_format = (scalability_mode >= 7) ? source_format_old + 1 : SF_CUSTOM;
	} else {
	    s->curr_reference_recon = s->prev_recon;
	    s->curr_image = s->prev_image;
	}

	s->curr_enhancement_recon = InitImage(sed_pels * sed_lines);

	mv_outside_frame_old = mv_outside_frame;
	mv_outside_frame = ON;

	/* Code the current EI/EP picture. Remember to frame prev_enhancement recon */
	CodeOneEP(s->curr_image, s->prev_enhancement_image,
		  s->prev_enhancement_recon, QP, s->P_frameskip * orig_frameskip,
		  bits, s->pic, s->curr_reference_recon, s->curr_enhancement_recon);

	if (deblocking_filter) {
	    EdgeFilter(s->curr_enhancement_recon, s->pic);
	}
	CalculateStatistics(s->curr_image, s->curr_enhancement_recon, NULL, NULL, bits, QP, s->pic);

	if (NULL != s->prev_enhancement_image) {
	    FreeImage(s->prev_enhancement_image);
	    FreeImage(s->prev_enhancement_recon);
	}
	s->prev_enhancement_image = s->curr_image;
	s->prev_enhancement_recon = s->curr_enhancement_recon;

	mv_outside_frame = mv_outside_frame_old;

	if (scalability_mode >= 3) {
	    s->pic->source_format = source_format_old;
	}
	break;

    default:
	fprintf(stderr,"tmn: oops - default switch\n");

	break;
    }

    /* Update full extended PTYPE  */
    if (EPTYPE) {
	/* UFEP is set once every 5 sec or once every five pictures *
	 * whichever is a larger time interval                       */
	if ((frames % 5 && (s->TR_old - s->TR_new) / s->pic->src_frame_rate >= 5)
	    || (s->TR_old - s->TR_new) / s->pic->src_frame_rate >= 5) {
	    s->TR_new = s->pic->TRB;
	    s->pic->UFEP = ON;
	} else {
	    s->pic->UFEP = OFF;
	}
	s->TR_old = s->pic->TRB;
    }
    if (s->pic->PB) {
	if (write_repeated)
	    wcopies = s->P_frameskip;
	for (i = 0; i < wcopies; i++)
	    WriteImage(s->B_recon, outputfile);	/* write wcopies frames to disk */
    }
    if (write_repeated) {
	wcopies = (pb_frames ? s->P_frameskip : frameskip);
    }
    for (i = 0; i < wcopies; i++)
	if (PCT_EI == s->pic->picture_coding_type ||
	    PCT_EP == s->pic->picture_coding_type) {
	    /* write wcopies frames to disk */
	    WriteImage(s->curr_enhancement_recon, outputfile_enhance);
	} else {
	    /* write wcopies frames to disk */
	    WriteImage(s->curr_recon, outputfile);
	}

    /* Can immediately free true B images, as the are never used for prediction */
    if (successive_B_frames && PCT_B == s->pic->picture_coding_type) {
	FreeImage(s->curr_image);
	FreeImage(s->curr_recon);
    }
    s->P_frameskip = (successive_B_frames) ? (successive_B_frames + 1) * s->fixed_frameskip : frameskip;

    prev_pict_type = s->pic->picture_coding_type;
    return 0;
}

void
h263_cleanup(struct ENCODER_STATE *s)
{
#ifndef VIC
    /* Closing files */
    fclose(streamfile);
    if (trace) {
	/*fclose(tf);*/
    }
#endif
    /* Free memory */
    free(streamname);
    free(seqfilename);
    free(outputfile);
    free(tracefile);
    free(enhance_seqfilename);

    if (pb_frames || improved_pb_frames) {
	free(pb_bits);
	free(pb_res);
	if (s->pic->PB) {
	    FreeImage(s->B_image);
	    FreeImage(s->B_recon);
	}
    }
    free(s->pic);

    free(bits);
    free(res);

    free(total_bits);
    free(total_res);

    free(intra_bits);
    free(intra_res);

    free(inter_bits);
    free(inter_res);

    if (successive_B_frames) {
	FreeImage(s->prev_P_image);
	FreeImage(s->prev_P_recon);
	FreeImage(s->next_P_image);
	FreeImage(s->next_P_recon);
	free(b_bits);
	free(b_res);
    }
    if (scalability_mode) {
	FreeImage(s->prev_enhancement_image);
	FreeImage(s->prev_enhancement_recon);
	free(scal_bits);
	free(scal_res);
    } else if (PCT_B != prev_pict_type) {
	FreeImage(s->curr_image);
	FreeImage(s->curr_recon);
    }
}

#ifndef VIC 
void
main(int argc, char *argv[])
{
    struct ENCODER_STATE *s;
    int i;

    fprintf(stdout, "\nTMN (H.263) coder version 3.0, University of British Columbia, CANADA, based on Telenor's coder version 2.0, Copyright (C) 1995, 1996 Telenor R&D, Norway\n");

    s = h263_init_encoder_1();
    ProcessArguments(argc, argv, s->pic);
    h263_init_encoder_2(s);

/***** Main loop *****/
    for (frame_no = start, i = 0; i <= (end - start) && frame_no <= end; ++i) {
	if (-1 == h263_encode_one_frame(s,i,0))
	    break;
    }
/***** end of main loop *****/

    /* Print total results */
    total_frames_passed = frame_no - start - first_frameskip;

    fprintf(stdout, "\n==== TOTAL ====\n");
    fprintf(stdout, "for %d images of %s\n", frames, seqfilename);

    if (frames != 0) {
	if (write_repeated)
	    fprintf(stdout, "Frames saved : %d predicted + %d intra\n",
		    total_frames_passed, icopies);

	fprintf(stdout, "--------------\n");

	if ((pb_frames || improved_pb_frames) && bframes != 0) {
	    fprintf(stdout, "SNR for %d B-frames:\n", bframes);
	    PrintSNR(pb_res, bframes);
	}
	if (successive_B_frames && Bframes != 0) {
	    fprintf(stdout, "SNR for %d true B-frames:\n", Bframes);
	    PrintSNR(b_res, Bframes);
	}
	if (scalability_mode && EIframes != 0) {
	    fprintf(stdout, "SNR for %d EI frames:\n", EIframes);
	    PrintSNR(ei_res, EIframes);
	}
	if (scalability_mode && EPframes != 0) {
	    fprintf(stdout, "SNR for %d EP frames:\n", EPframes);
	    PrintSNR(ep_res, EPframes);
	}
	fprintf(stdout, "SNR for %d P-frames:\n", pframes);
	PrintSNR(inter_res, pframes);

	if ((pb_frames || improved_pb_frames) && bframes != 0) {
	    fprintf(stdout, "SNR for total %d frames:\n", pframes + bframes);
	    fprintf(stdout, "SNR_Y  : %.2f\n", (pb_res->SNR_l + inter_res->SNR_l) / (bframes + pframes));
	    fprintf(stdout, "SNR_Cb : %.2f\n", (pb_res->SNR_Cb + inter_res->SNR_Cb) / (bframes + pframes));
	    fprintf(stdout, "SNR_Cr : %.2f\n", (pb_res->SNR_Cr + inter_res->SNR_Cr) / (bframes + pframes));
	    fprintf(stdout, "--------------\n");
	}
	if (successive_B_frames && Bframes != 0) {
	    fprintf(stdout, "SNR for total %d frames:\n", pframes + Bframes);
	    fprintf(stdout, "SNR_Y  : %.2f\n", (b_res->SNR_l + inter_res->SNR_l) / (Bframes + pframes));
	    fprintf(stdout, "SNR_Cb : %.2f\n", (b_res->SNR_Cb + inter_res->SNR_Cb) / (Bframes + pframes));
	    fprintf(stdout, "SNR_Cr : %.2f\n", (b_res->SNR_Cr + inter_res->SNR_Cr) / (Bframes + pframes));
	    fprintf(stdout, "--------------\n");
	}
	if (scalability_mode) {
	    fprintf(stdout, "SNR for total %d enhancement frames:\n", EIframes + EPframes);
	    fprintf(stdout, "SNR_Y  : %.2f\n", (ei_res->SNR_l + ep_res->SNR_l) / (EIframes + EPframes));
	    fprintf(stdout, "SNR_Cb : %.2f\n", (ei_res->SNR_Cb + ep_res->SNR_Cb) / (EIframes + EPframes));
	    fprintf(stdout, "SNR_Cr : %.2f\n", (ei_res->SNR_Cr + ep_res->SNR_Cr) / (EIframes + EPframes));
	    fprintf(stdout, "--------------\n");
	}
	fprintf(stdout, "Bit totals for %d(%d) base layer frames:\n", frames, pframes);
	PrintResult(total_bits, pframes, frames);

	if (scalability_mode) {
	    fprintf(stdout, "Bit totals for %d(%d) enhancement layer frames:\n", EIframes + EPframes, EPframes);
	    PrintResult(scal_bits, EIframes + EPframes, EIframes + EPframes);
	}
	if (targetrate != 0 || s->pic->bit_rate != 0)
	    fprintf(stdout, "Original seq time: %.2f (%.2f) sec\n",
		    (total_frames_passed + first_frameskip) /
		    ref_frame_rate * orig_frameskip,
		    total_frames_passed /
		    ref_frame_rate * orig_frameskip);

	fprintf(stdout, "Mean quantizer for inter frames : %.2f\n", inter_res->QP_mean / pframes);

	if (scalability_mode) {
	    fprintf(stdout, "Mean quantizer for enhancement frames : %.2f\n",
		    scal_res->QP_mean / (EIframes + EPframes));
	}
#if 0
	fprintf(stdout, "Total frames     : %3d (%3d)\n",
		total_frames_passed + first_frameskip,
		total_frames_passed);
#endif

	fprintf(stdout, "Encoded frames   : %3d (%3d)\n",
		frames + 1,
		frames);

	if (scalability_mode) {
	    fprintf(stdout, "Encoded frames   : %3d (%3d)\n",
		    EIframes + EPframes,
		    EPframes);
	}
	mean_frame_rate = frames / (float) total_frames_passed *
	                ref_frame_rate / (float) orig_frameskip;

	fprintf(stdout, "Mean frame rate  : %.2f Hz\n", mean_frame_rate);

	if (targetrate != 0)
	    fprintf(stdout, "Target bit rate  : %.2f kbit/sec\n",
		    targetrate / 1000.0);

	fprintf(stdout, "Obtained bit rate: %.2f (%.2f) kbit/sec\n",
		(total_bits->total + intra_bits->total) /
		((total_frames_passed + first_frameskip) /
		 ref_frame_rate * orig_frameskip) / 1000.0,
	(total_bits->total / (float) frames) * mean_frame_rate / 1000.0);

	if (scalability_mode) {
	    fprintf(stdout, "Obtained enhncement layer bit rate: %.2f kbit/sec\n",
		    (scal_bits->total) /
		    ((total_frames_passed + first_frameskip) /
		     ref_frame_rate * orig_frameskip) / 1000.0);
	}
	fprintf(stdout, "============================================\n");

    }
#if 0
    fprintf(stdout, "Total number of bits: %d (%d)\n",
	    total_bits->total + intra_bits->total,
	    (total_bits->total + intra_bits->total) / 8);
#endif

    h263_cleanup(s);
    exit(0);
}
#endif /* VIC */

/**********************************************************************
 *
 *	Name:        Help
 *	Description:        Prints usage
 *	
 *
 ***********************************************************************/

void
Help()
{
    fprintf(stdout, "Usage:\tmn [options] -i <filename> [more options]\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "\t-i <filename> original sequence [required parameter]\n");
    fprintf(stdout, "\t-o <filename> reconstructed frames [%s]\n",
	    DEF_OUTFILENAME);
    fprintf(stdout, "\t-B <filename> filename for bitstream [%s]\n",
	    DEF_STREAMNAME);
    fprintf(stdout, "\t-a <n> image to start at [%d]\n",
	    DEF_START_FRAME);
    fprintf(stdout, "\t-b <n> image to stop at [%d]\n",
	    DEF_STOP_FRAME);
    fprintf(stdout, "\t-x <n> (<sed_pels> <sed_lines>) coding format [%d]\n", DEF_CODING_FORMAT);
    fprintf(stdout, "\t       n=1: SQCIF n=2: QCIF n=3: CIF n=4: 4CIF n=5: 16CIF  n=6:  Custom (12:11 PAR)\n");
    fprintf(stdout, "\t           128x96   176x144  352x288   704x576  1408x1152  sed_pels x sed_lines\n");
    fprintf(stdout, "\t-s <n> (0..15) integer pel search window [%d]\n",
	    DEF_SEEK_DIST);
    fprintf(stdout, "\t-q <n> (1..31) quantization parameter QP [%d]\n",
	    DEF_INTER_QUANT);
    fprintf(stdout, "\t-A <n> (1..31) QP for first frame [%d]\n",
	    DEF_INTRA_QUANT);
    fprintf(stdout, "\t-r <n> target bitrate in bits/s, default is variable bitrate\n");
    fprintf(stdout, "\t -C <n> Rate control method [%d]\n",
	    DEF_RC_METHOD);
    fprintf(stdout, "\t-k <n> frames to skip between each encoded frame [%d]\n",
	    DEF_FRAMESKIP);
    fprintf(stdout, "\t-Z <n> reference frame rate (25 or 30 fps) [%.1f]\n",
	    DEF_REF_FRAME_RATE);
    fprintf(stdout, "\t-l <n> frames skipped in original compared to reference frame rate [%d]\n", DEF_ORIG_SKIP);
    fprintf(stdout, "\t-e <n> original sequence has n bytes header [%d]\n",
	    DEF_HEADERLENGTH);
    fprintf(stdout, "\t-g <n> insert sync after each n GOB (slice) [%d]\n",
	    DEF_INSERT_SYNC);
    fprintf(stdout, "\t       zero above means no extra syncs inserted\n");
    fprintf(stdout, "\t-w write difference image to file \"%s\" [%s]\n",
	    DEF_DIFFILENAME,
	    DEF_WRITE_DIFF ? "ON" : "OFF");
    fprintf(stdout, "\t-m write repeated reconstructed frames to disk [%s]\n",
	    DEF_WRITE_REPEATED ? "ON" : "OFF");
    fprintf(stdout, "\t-t write trace to tracefile trace.intra/trace [%s]\n",
	    DEF_WRITE_TRACE ? "ON" : "OFF");
    fprintf(stdout, "\t-D <n> use unrestricted motion vector mode (annex D) [%s]\n",
	    DEF_UMV_MODE ? "ON" : "OFF");
    fprintf(stdout, "\t       n=1: H.263 n=2: H.263+\n");
    fprintf(stdout, "\t-E use syntax-based arithmetic coding (annex E) [%s]\n",
	    DEF_SAC_MODE ? "ON" : "OFF");
    fprintf(stdout, "\t-F use advanced prediction mode (annex F) [%s]\n",
	    DEF_ADV_MODE ? "ON" : "OFF");
    fprintf(stdout, "\t-G use PB-frames (annex G) [%s]\n",
	    DEF_PBF_MODE ? "ON" : "OFF");
    fprintf(stdout, "\t   -U <n> (0..3) BQUANT parameter [%d]\n", DEF_BQUANT);
    fprintf(stdout, "\t-M use improved PB-frames (annex M) [%s]\n",
	    DEF_PBF_MODE ? "ON" : "OFF");
    fprintf(stdout, "\t-I use advanced intra coding mode (annex I) [%s]\n",
	    DEF_AIC_MODE ? "ON" : "OFF");
    fprintf(stdout, "\t-J use deblocking filter (annex J) [%s]\n",
	    DEF_DF_MODE ? "ON" : "OFF");
/*fprintf(stdout,"\t-K use slice structure (annex K) [%s]\n",
   DEF_SS_MODE ? "ON" : "OFF");
   fprintf(stdout,"\t-N use reference picture selection mode (annex N) [%s]\n",
   DEF_RPS_MODE ? "ON" : "OFF"); */
    fprintf(stdout, "\t-c <n> frames to select number of true B pictures between P pictures (annex O) [%d]\n",
	    DEF_TEMP_SCAL);
    fprintf(stdout, "\t-d <n> to set QP for true B pictures (annex O) [%d]\n",
	    DEF_TRUE_B_QUANT);
    fprintf(stdout, "\t-i <filename> enhancement layer sequence\n");
    fprintf(stdout, "\t-u <n> to select SNR or spatial scalability mode (annex O) [%s]\n",
	    DEF_SNR_SPAT_SCAL_MODE ? "ON" : "OFF");
    fprintf(stdout, "\t       n=1: SNR n=3: SPATIAL(horiz) n=5: SPATIAL(vert) n=7: SPATIAL(both)\n");
    fprintf(stdout, "\t-v <n> to set QP for enhancement layer (annex O) [%d]\n",
	    DEF_EI_EP_QUANT);
/*  fprintf(stdout,"\t-O use SNR/spatial scalability (annex O) [%s]\n",
   DEF_SNRSS_TYPE ? "ON" : "OFF");
   fprintf(stdout,"\t-P use reference picture resampling mode (annex P) [%s]\n",
   DEF_RPR_MODE ? "ON" : "OFF");
   fprintf(stdout,"\t-Q use reduced resolution update mode (annex Q) [%s]\n",
   DEF_RRU_MODE ? "ON" : "OFF");
   fprintf(stdout,"\t-R use  independently segmented decoding mode (annex R) [%s]\n",
   DEF_ISD_MODE ? "ON" : "OFF"); */
    fprintf(stdout, "\t-S use alternative inter vlc mode (annex S) [%s]\n",
	    DEF_AIV_MODE ? "ON" : "OFF");
    fprintf(stdout, "\t-T use modified quantization mode (annex T) [%s]\n",
	    DEF_MQ_MODE ? "ON" : "OFF");

    fprintf(stdout, "\t-h Prints help\n");
    fprintf(stdout, "\n\tDefault filenames and other options in square brackets \n\tare chosen in config.h\n");
    return;
}

/**********************************************************************
 *
 *	Name:        PrintResult
 *	Description:	add bits and prints results
 *	
 *	Input:        Bits struct
 *        
 *	Returns:	
 *	Side effects:	
 *
 *	Date: 940116	Author:	Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void
PrintResult(Bits * bits, int num_units, int num)
{
    fprintf(stdout, "# intra   : %d\n", bits->no_intra / num_units);
    fprintf(stdout, "# inter   : %d\n", bits->no_inter / num_units);
    fprintf(stdout, "# inter4v : %d\n", bits->no_inter4v / num_units);
    fprintf(stdout, "--------------\n");
    fprintf(stdout, "Coeff_Y: %d\n", bits->Y / num);
    fprintf(stdout, "Coeff_C: %d\n", bits->C / num);
    fprintf(stdout, "Vectors: %d\n", bits->vec / num);
    fprintf(stdout, "CBPY   : %d\n", bits->CBPY / num);
    fprintf(stdout, "MCBPC  : %d\n", bits->CBPCM / num);
    fprintf(stdout, "MODB   : %d\n", bits->MODB / num);
    fprintf(stdout, "CBPB   : %d\n", bits->CBPB / num);
    fprintf(stdout, "COD    : %d\n", bits->COD / num);
    fprintf(stdout, "DQUANT : %d\n", bits->DQUANT / num);
    if (advanced_intra_coding)
	fprintf(stdout, "INTRA_MODE: %d\n", bits->INTRA_MODE / num);
    fprintf(stdout, "header : %d\n", bits->header / num);
    fprintf(stdout, "==============\n");
    fprintf(stdout, "Total  : %d\n", bits->total / num);
    fprintf(stdout, "\n");
    return;
}

void
PrintSNR(Results * res, int num)
{
    fprintf(stdout, "SNR_Y  : %.2f\n", res->SNR_l / num);
    fprintf(stdout, "SNR_Cb : %.2f\n", res->SNR_Cb / num);
    fprintf(stdout, "SNR_Cr : %.2f\n", res->SNR_Cr / num);
    fprintf(stdout, "--------------\n");
    return;
}

void
InitializeCoder(Pict * pic)
{
    /* Default variable values */
    adv_pred = DEF_ADV_MODE;
    syntax_arith_coding = DEF_SAC_MODE;
    pic->unrestricted_mv_mode = DEF_UMV_MODE;
    mv_outside_frame = DEF_UMV_MODE || DEF_ADV_MODE;
    long_vectors = DEF_UMV_MODE;
    pb_frames = DEF_PBF_MODE;

    /* h263+ options */
    improved_pb_frames = DEF_IM_PBF_MODE;
    advanced_intra_coding = DEF_AIC_MODE;
    deblocking_filter = DEF_DF_MODE;
    slice_structure = DEF_SS_MODE;
    pic->reference_picture_selection = DEF_RPS_MODE;
    independently_segmented_decoding = DEF_ISD_MODE;
    alternative_inter_vlc = DEF_AIV_MODE;
    modified_quantization = DEF_MQ_MODE;
    scalability_mode = DEF_SNR_SPAT_SCAL_MODE;
    reference_picture_resampling = DEF_RPR_MODE;
    reduced_resolution_update = DEF_RRU_MODE;
    successive_B_frames = DEF_TEMP_SCAL;
    b_picture_quant = DEF_TRUE_B_QUANT;
    enhancement_layer_quant = DEF_EI_EP_QUANT;
    B_pictures_to_code = 0;

    pic->RLNUM = BASE_LAYER_NUM;
    pic->ELNUM = BASE_LAYER_NUM;

    pic->BQUANT = DEF_BQUANT;
    pic->source_format = DEF_CODING_FORMAT;
    rate_control_method = DEF_RC_METHOD;
    start_rate_control = DEF_START_RATE_CONTROL;

    pic->seek_dist = DEF_SEEK_DIST;
    pic->use_gobsync = DEF_INSERT_SYNC;

    QP = DEF_INTER_QUANT;
    QPI = DEF_INTRA_QUANT;

    ref_frame_rate = (float) DEF_REF_FRAME_RATE;
    chosen_frameskip = DEF_FRAMESKIP + 1;
    orig_frameskip = DEF_ORIG_SKIP + 1;

    seqfilename = (char *) malloc(sizeof(char) * 100);
    enhance_seqfilename = (char *) malloc(sizeof(char) * 100);
    streamname = (char *) malloc(sizeof(char) * 100);
    outputfile = (char *) malloc(sizeof(char) * 100);
    outputfile_enhance = (char *) malloc(sizeof(char) * 100);

    diff_filename = DEF_DIFFILENAME;
    tracefile = (char *) malloc(sizeof(char) * 100);

    seqfilename[0] = '\0';
    enhance_seqfilename[0] = '\0';
    strcpy(streamname, DEF_STREAMNAME);
    strcpy(outputfile, DEF_OUTFILENAME);
    strcpy(outputfile_enhance, DEF_OUTFILENAME_ENHANCE);

    mv_outside_frame_old = DEF_UMV_MODE;
    writediff = DEF_WRITE_DIFF;
    trace = DEF_WRITE_TRACE;
    write_repeated = DEF_WRITE_REPEATED;
    prev_I_P_pic_TR = start = DEF_START_FRAME;
    end = DEF_STOP_FRAME;

    targetrate = 0;
    /* default is variable bit rate (fixed quantizer) will be used */

    frames = 0;
    pframes = 0;
    bframes = 0;
    Bframes = 0;
    total_frames_passed = 0;
    pic->PB = 0;
    wcopies = icopies = 1;

    headerlength = DEF_HEADERLENGTH;

    pic->TR = 0;
    pic->QP_mean = (float) 0.0;
    EPTYPE = 0;
    pic->UFEP = 1;
    pic->PCF = 0;
    pic->TRPI = 0;
    pic->PEI = 0;
    pic->MODB = 0;
}

void
ProcessArguments(int argc, char *argv[], Pict * pic)
{
    int             i;

    for (i = 1; i < argc; i++) {
	if (*(argv[i]) == '-') {
	    switch (*(++argv[i])) {
	    case 'a':
		prev_I_P_pic_TR = start = atoi(argv[++i]);
		break;
	    case 'b':
		end = atoi(argv[++i]);
		break;
	    case 'c':
		successive_B_frames = atoi(argv[++i]);
		EPTYPE = ON;
		break;
	    case 'd':
		b_picture_quant = atoi(argv[++i]);
		break;
	    case 'e':
		headerlength = atoi(argv[++i]);
		break;
	    case 'g':
		pic->use_gobsync = atoi(argv[++i]);;
		break;
	    case 'H':
	    case 'h':
		Help();
		exit(0);
		break;
	    case 'i':
		strcpy(seqfilename, argv[++i]);
		break;
	    case 'k':
		chosen_frameskip = atoi(argv[++i]) + 1;
		break;
	    case 'l':
		orig_frameskip = atoi(argv[++i]) + 1;
		break;
	    case 'm':
		write_repeated = ON;
		break;
	    case 'o':
		strcpy(outputfile, argv[++i]);
		break;
	    case 'q':
		QP = atoi(argv[++i]);
		if (QP > 31 || QP < 0) {
		    fprintf(stderr, "QP out of range - clipping it\n");
		    QP = mmin(31, mmax(0, QP));
		}
		break;
	    case 'r':
		targetrate = atoi(argv[++i]);
		break;
	    case 's':
		pic->seek_dist = atoi(argv[++i]);
		break;
	    case 't':
		trace = 1;
		break;
	    case 'u':
		scalability_mode = atoi(argv[++i]);
		EPTYPE = ON;
		break;
	    case 'v':
		enhancement_layer_quant = atoi(argv[++i]);
		break;
	    case 'w':
		writediff = ON;
		break;
	    case 'x':
		pic->source_format = atoi(argv[++i]);
		if (pic->source_format == SF_CUSTOM) {
		    EPTYPE = 1;
		    sed_pels = atoi(argv[++i]);
		    sed_lines = atoi(argv[++i]);
		}
		break;
	    case 'A':
		QPI = atoi(argv[++i]);
		if (QPI > 31 || QPI < 0) {
		    fprintf(stderr, "QP out of range - clipping it\n");
		    QPI = mmin(31, mmax(0, QPI));
		}
		break;
	    case 'B':
		strcpy(streamname, argv[++i]);

		break;
	    case 'C':
		rate_control_method = atoi(argv[++i]);
		break;
	    case 'D':
		/* note that the Unrestricted Motion Vector mode turns on 
		 * both long_vectors and mv_outside_frame */
		pic->unrestricted_mv_mode = ON;
		mv_outside_frame = ON;
		long_vectors = ON;
		if (atoi(argv[++i]) == 2) {
		    /* use H.263+ Annex D (RVLC) */
		    EPTYPE = ON;
		}
		break;
	    case 'E':
		syntax_arith_coding = ON;
		break;
	    case 'F':
		/* note that the Advanced Prediction mode turns on both 
		 * advanced (8x8 vectors and OBMC) and mv_outside_frame */
		/* the Extended Motion Vector mode is included in the
		 * Unrestricted Motion Vector mode, which of course can be
		 * use together with the Advanced Prediction mode */
		overlapping_MC = ON;
		adv_pred = ON;
		use_4mv = ON;
		mv_outside_frame = ON;
		break;
	    case 'G':
		pb_frames = ON;
		break;
	    case 'I':
		advanced_intra_coding = ON;
		EPTYPE = ON;
		break;
	    case 'J':
		/* see table J.1 in H.263+ standard for dependencies */
		deblocking_filter = ON;
		mv_outside_frame = ON;
		use_4mv = ON;
		EPTYPE = ON;
		break;
	    case 'K':
		slice_structure = ON;
		EPTYPE = ON;
		break;
	    case 'M':
		improved_pb_frames = ON;
		EPTYPE = ON;
		break;
	    case 'N':
		pic->reference_picture_selection = ON;
		EPTYPE = ON;
		break;
	    case 'O':
		strcpy(enhance_seqfilename, argv[++i]);
		break;
	    case 'P':
		reference_picture_resampling = ON;
		EPTYPE = ON;
		break;
	    case 'Q':
		reduced_resolution_update = ON;
		EPTYPE = ON;
		break;
	    case 'R':
		independently_segmented_decoding = ON;
		EPTYPE = ON;
		break;
	    case 'S':
		alternative_inter_vlc = ON;
		EPTYPE = ON;
		break;
	    case 'T':
		modified_quantization = ON;
		EPTYPE = ON;
		break;
	    case 'U':
		pic->BQUANT = atoi(argv[++i]);
		break;
	    case 'Z':
		ref_frame_rate = (float) atoi(argv[++i]);
		break;
	    default:
		fprintf(stderr, "Illegal option: %c\n", *argv[i]);
		Help();
		exit(-1);
		break;
	    }
	}
    }

    switch (pic->source_format) {
    case (SF_SQCIF):
	fprintf(stdout, "Encoding format: SQCIF (128x96)\n");
	sed_pels = 128;
	sed_lines = 96;
	break;
    case (SF_QCIF):
	fprintf(stdout, "Encoding format: QCIF (176x144)\n");
	sed_pels = 176;
	sed_lines = 144;
	break;
    case (SF_CIF):
	fprintf(stdout, "Encoding format: CIF (352x288)\n");
	sed_pels = 352;
	sed_lines = 288;
	break;
    case (SF_4CIF):
	fprintf(stdout, "Encoding format: 4CIF (704x576)\n");
	sed_pels = 704;
	sed_lines = 576;
	break;
    case (SF_16CIF):
	fprintf(stdout, "Encoding format: 16CIF (1408x1152)\n");
	sed_pels = 1408;
	sed_lines = 1152;
	break;
    case (SF_CUSTOM):
	if ((sed_pels % 16) || (sed_lines % 16)) {
	    fprintf(stdout, "Error: only custom format multiple of 16 supported\n");
	    exit(-1);
	} else {
	    fprintf(stdout, "Encoding format: custom (%dx%d)\n", sed_pels, sed_lines);
	    break;
	}
    default:
	fprintf(stderr, "Illegal coding format\n");
	exit(-1);
    }

    csed_pels = sed_pels / 2;

    base_sed_pels = sed_pels;
    base_sed_lines = sed_lines;

    enhancement_sed_pels = sed_pels * scal_tab[0][scalability_mode];
    enhancement_sed_lines = sed_lines * scal_tab[1][scalability_mode];

    if (seqfilename[0] == '\0') {
	fprintf(stderr, "Required input parameter \'-i <filename>\' missing\n");
	Help();
	exit(-1);
    }
    if ((scalability_mode >= 3) && enhance_seqfilename[0] == '\0') {
	fprintf(stderr, "No enhancement layer input sequence has been specified \'-O <filename>\'.\n");
	fprintf(stderr, "Ehancement layer sequence will be generated from base layer sequence.\n");
    }
    /* Currently, cannot use frame rate control when using temporal
       scalability. */
    if (successive_B_frames && (targetrate > 0)) {
	fprintf(stderr, "Warning:");
	fprintf(stderr, "Variable frame rate and temporal scalability not supported\n");
	fprintf(stderr, "Resetting to fixed frame rate\n");
	targetrate = 0;
    }
    if (successive_B_frames && (pb_frames || improved_pb_frames)) {
	fprintf(stderr, "Warning:");
	fprintf(stderr, "Encoder supports either PB frames or true B pictures, not both.\n");
	exit(-1);
    }
    if (successive_B_frames && scalability_mode) {
	fprintf(stderr, "Warning:");
	fprintf(stderr, "Encoder does not yet support multiple scalability options.\n");
	exit(-1);
    }
    /* check for mode restrictions */
    if (syntax_arith_coding && (alternative_inter_vlc ||
			  modified_quantization || improved_pb_frames)) {
	fprintf(stderr, "syntax_arith_coding cannot be used with alternative_inter_vlc, improved pb or  modified_quantization options\n");
	exit(-1);
    }
    /* Open stream for writing */
    streamfile = fopen(streamname, "wb");
    if (streamname == NULL) {
	fprintf(stderr, "Unable to open streamfile\n");
	exit(-1);
    }
    /* Clear output files */
    if ((cleared = fopen(outputfile, "wb")) == NULL) {
	fprintf(stderr, "Couldn't open outputfile: %s\n", outputfile);
	exit(-1);
    } else {
	fclose(cleared);
    }

    if (writediff) {
	if ((cleared = fopen(diff_filename, "wb")) == NULL) {
	    fprintf(stderr, "Couldn't open diff-file: %s\n", diff_filename);
	    exit(-1);
	} else {
	    fclose(cleared);
	}
    }
    if (QP == 0 || QPI == 0) {
	fprintf(stderr, "Warning:");
	fprintf(stderr, "QP is zero. Bitstream will not be correctly decodable\n");
    }
    if (ref_frame_rate != 25.0 && ref_frame_rate != 30.0) {
	fprintf(stderr, "Warning: Reference frame rate should be 25 or 30 fps\n");
    }
    /* pb frames and improved pb frames can't be used together */
    if (pb_frames && improved_pb_frames) {
	fprintf(stdout, "PB frames option can't be used with Improved pb frames option\n");
	fprintf(stdout, "PB frames option is ignored\n");
    }
    pic->target_frame_rate = (ref_frame_rate /
			      (orig_frameskip * chosen_frameskip));

    return;
}

void
DeterminePictureType(int *frame_no, Pict * pic, int P_skip, int B_skip, int i)
{

    if (0 == i) {
	/* Intra */
	pic->picture_coding_type = PCT_INTRA;
	*frame_no = prev_I_P_frame_no = start;
	prev_I_P_pic_TR = pic->TR = prev_I_P_pic_TR;
    } else {
	if ((scalability_mode) && (PCT_INTER == prev_pict_type || PCT_INTRA == prev_pict_type)) {
	    /* EI or EP pictures */
	    pic->picture_coding_type = (PCT_INTRA == prev_pict_type) ? PCT_EI : PCT_EP;
	    *frame_no = prev_I_P_frame_no;
	    pic->TR = prev_I_P_pic_TR;
	} else {
	    if (successive_B_frames) {
		if (B_pictures_to_code) {
		    /* B picture */
		    pic->picture_coding_type = PCT_B;
		    *frame_no = prev_I_P_frame_no - (B_pictures_to_code * B_skip);
		    --B_pictures_to_code;
		    pic->TR = (prev_I_P_pic_TR + ((successive_B_frames - B_pictures_to_code) *
				       (B_skip * orig_frameskip))) % 256;
		    pic->TRB = pic->TR - prev_I_P_pic_TR;
		    TRP = P_skip;
		} else {
		    /* P picture */
		    if (PCT_B == prev_pict_type) {
			prev_I_P_pic_TR = pic->TR + B_skip;
		    }
		    B_pictures_to_code = successive_B_frames;
		    if (prev_I_P_frame_no + P_skip <= end) {
			pic->picture_coding_type = PCT_INTER;
			*frame_no = prev_I_P_frame_no = prev_I_P_frame_no + P_skip;
			pic->TR = (prev_I_P_pic_TR +
				   (P_skip * orig_frameskip)) % 256;
		    } else {
			*frame_no = prev_I_P_frame_no;
			pic->picture_coding_type = -1;
			pic->TR = prev_I_P_pic_TR;
		    }
		}
	    } else {
		if (pb_frames || improved_pb_frames) {
		    if (prev_I_P_frame_no + P_skip > end) {
			*frame_no = prev_I_P_frame_no;
			pic->picture_coding_type = -1;
			pic->TR = prev_I_P_pic_TR;
		    } else {
			if (PCT_INTRA == prev_pict_type || P_skip > 8 || (prev_I_P_frame_no + 2 * P_skip) > end) {	/* If previous picture is INTRA || the temporal reference of P thus
															   the temporal reference of B is larger than 8 || there are not enough
															   frames left to code as PB than code the next picture as P picture */
			    pic->PB = 0;
			    pic->picture_coding_type = PCT_INTER;
			    *frame_no = prev_I_P_frame_no = prev_I_P_frame_no + P_skip;
			    prev_I_P_pic_TR = pic->TR = (prev_I_P_pic_TR +
					(P_skip * orig_frameskip)) % 256;
			} else {	/* PB or IPB picture */
			    pic->PB = (improved_pb_frames) ? IM_PB_FRAMES : PB_FRAMES;
			    pic->picture_coding_type = (improved_pb_frames) ? PCT_IPB : PCT_PB;
			    *frame_no = prev_I_P_frame_no + P_skip;
			    prev_I_P_frame_no += 2 * P_skip;
			    prev_I_P_pic_TR = pic->TR = (prev_I_P_pic_TR +
				    (2 * P_skip * orig_frameskip)) % 256;
			}
		    }
		} else {
		    /* P picture */
		    pic->picture_coding_type = PCT_INTER;
		    *frame_no = prev_I_P_frame_no = prev_I_P_frame_no + P_skip;
		    prev_I_P_pic_TR = pic->TR = (prev_I_P_pic_TR +
					(P_skip * orig_frameskip)) % 256;
		}
	    }
	}
    }
#ifndef VIC
    if (*frame_no > end) {
	pic->picture_coding_type = -1;
	pic->TR = prev_I_P_pic_TR;
    }
#endif
}

void
CalculateStatistics(PictImage * curr_image, PictImage * curr_recon,
		    PictImage * pb_b_image, PictImage * pb_b_recon,
		    Bits * bits, int QP, Pict * pic)
{
    //SV-XXX: dummy initialisations to make gcc-4.0 shut up
    UNUSED(pb_b_image);
    UNUSED(pb_b_recon);
    UNUSED(QP);

    fprintf(stdout, "done\n");

    ComputeSNR(curr_image, curr_recon, res, pic->picture_coding_type, writediff);

    fflush(stdout);

    if (arith_used) {
	bits->header += encoder_flush();
	arith_used = 0;
    }
    bits->header += alignbits();	/* pictures shall be byte aligned */

    AddBitsPicture(bits);

#ifndef VIC
    switch (pic->picture_coding_type) {
    case PCT_INTRA:

	fprintf(stdout, "Average Intra QP: %d\n", QP);
	AddRes(intra_res, res, pic);

	AddBits(intra_bits, bits);

	break;

    case PCT_INTER:

	fprintf(stdout, "Results for P-frame:\n");
	AddRes(inter_res, res, pic);

	fprintf(stdout, "Average Inter QP: %5.2f\n", pic->QP_mean);

	AddBits(inter_bits, bits);
	AddBits(total_bits, bits);

	break;

    case PCT_PB:
    case PCT_IPB:

	fprintf(stdout, "Results for P component of PB frame:\n");
	AddRes(inter_res, res, pic);
	PrintSNR(res, 1);

	ComputeSNR(pb_b_image, pb_b_recon, res, pic->picture_coding_type, writediff);

	fprintf(stdout, "Results for B component of PB frame:\n");
	AddRes(pb_res, res, pic);

	fprintf(stdout, "Average P component QP: %d\n", QP);
	fprintf(stdout, "Average B component BQUANT: %d\n", pic->BQUANT);

	AddBits(pb_bits, bits);
	AddBits(total_bits, bits);

	break;

    case PCT_B:

	fprintf(stdout, "Results for true B-frame:\n");
	AddRes(b_res, res, pic);

	fprintf(stdout, "Average B QP: %d\n", QP);

	AddBits(b_bits, bits);
	AddBits(total_bits, bits);

	break;

    case PCT_EI:

	fprintf(stdout, "Results for EI frame:\n");
	AddRes(ei_res, res, pic);
	AddRes(scal_res, res, pic);

	fprintf(stdout, "Average enhancement layer (EI) QP: %d\n", QP);

	AddBits(ei_bits, bits);
	AddBits(scal_bits, bits);

	break;

    case PCT_EP:

	fprintf(stdout, "Results for EP frame:\n");
	AddRes(ep_res, res, pic);
	AddRes(scal_res, res, pic);

	fprintf(stdout, "Average enhancement layer (EP) QP: %d\n", QP);

	AddBits(ep_bits, bits);
	AddBits(scal_bits, bits);

	break;

    default:

	break;

    }

    PrintSNR(res, 1);
    PrintResult(bits, 1, 1);
#endif
}

PictImage      *
sed_UpsampleReferenceLayerPicture(PictImage * base_image)
{

    PictImage      *upsampled_image;
    int             size;

    size = enhancement_sed_pels * enhancement_sed_lines;

    if ((upsampled_image = (PictImage *) malloc(sizeof(PictImage))) == NULL) {
	fprintf(stderr, "Couldn't allocate (PictImage *)\n");
	exit(-1);
    }
    if ((upsampled_image->lum = (unsigned char *) malloc(sizeof(char) * size)) == NULL) {
	fprintf(stderr, "Couldn't allocate memory for luminance\n");
	exit(-1);
    }
    if ((upsampled_image->Cr = (unsigned char *) malloc(sizeof(char) * size / 4)) == NULL) {
	fprintf(stderr, "Couldn't allocate memory for Cr\n");
	exit(-1);
    }
    if ((upsampled_image->Cb = (unsigned char *) malloc(sizeof(char) * size / 4)) == NULL) {
	fprintf(stderr, "Couldn't allocate memory for Cb\n");
	exit(-1);
    }
    sed_UpsampleComponent(base_image->lum, upsampled_image->lum,
		      base_sed_pels, base_sed_lines);
    sed_UpsampleComponent(base_image->Cr, upsampled_image->Cr,
		      base_sed_pels / 2, base_sed_lines / 2);
    sed_UpsampleComponent(base_image->Cb, upsampled_image->Cb,
		      base_sed_pels / 2, base_sed_lines / 2);

    return (upsampled_image);
}

void
sed_UpsampleComponent(unsigned char *base, unsigned char *enhanced,
		  int horiz, int vert)
{
    int             i, j;
    unsigned char  *base_next, *enhanced_next, *enhanced_origin;

    enhanced_origin = enhanced;

    switch (scalability_mode) {
    case SPATIAL_SCALABILITY_H:

	/* Rows */
	for (j = 0; j < vert; j++) {
	    /* First column of rows */
	    *enhanced++ = *base;
	    for (i = 1; i < horiz; i++) {
		*enhanced++ = (3 * *base + *(base + 1) + 2) >> 2;
		*enhanced++ = (*base + 3 * *(base + 1) + 2) >> 2;
		base++;
	    }
	    /* Last column of rows */
	    *enhanced++ = *base++;
	}

	break;

    case SPATIAL_SCALABILITY_V:

	/* First row */
	for (i = 0; i < horiz; i++) {
	    *enhanced++ = *base++;
	}

	enhanced_next = enhanced + horiz;
	base = base - horiz;
	base_next = base + horiz;

	/* Rows */
	for (j = 0; j < vert - 1; j++) {

	    for (i = 0; i < horiz; i++) {
		*enhanced++ = (3 * *base + *(base_next) + 2) >> 2;
		*enhanced_next++ = (*base + 3 * *(base_next) + 2) >> 2;
		base++;
		base_next++;
	    }
	    enhanced = enhanced + horiz;
	    enhanced_next = enhanced + horiz;
	}

	/* Last row */
	for (i = 0; i < horiz; i++) {
	    *enhanced++ = *base++;
	}

	break;

    case SPATIAL_SCALABILITY_HV:

	/* Top left corner pel */
	*enhanced++ = *base;
	/* First row */
	for (i = 1; i < horiz; i++) {
	    *enhanced++ = (3 * *base + *(base + 1) + 2) >> 2;
	    *enhanced++ = (*base + 3 * *(base + 1) + 2) >> 2;
	    base++;
	}
	/* Top right corner pel */
	*enhanced++ = *base++;

	enhanced_next = enhanced + (horiz << 1);
	base = base - horiz;
	base_next = base + horiz;

	/* Rows */
	for (j = 0; j < vert - 1; j++) {
	    /* First column of rows */
	    *enhanced++ = (3 * *base + *(base_next) + 2) >> 2;
	    *enhanced_next++ = (*base + 3 * *(base_next) + 2) >> 2;
	    for (i = 1; i < horiz; i++) {
		*enhanced++ = (9 * *base + 3 * *(base + 1) + 3 * *base_next +
			       *(base_next + 1) + 8) >> 4;
		*enhanced++ = (3 * *base + 9 * *(base + 1) + *base_next +
			       3 * *(base_next + 1) + 8) >> 4;
		*enhanced_next++ = (3 * *base + *(base + 1) + 9 * *base_next +
				    3 * *(base_next + 1) + 8) >> 4;
		*enhanced_next++ = (*base + 3 * *(base + 1) + 3 * *base_next +
				    9 * *(base_next + 1) + 8) >> 4;
		base++;
		base_next++;
	    }
	    /* Last column of rows */
	    *enhanced++ = (3 * *base + *(base_next) + 2) >> 2;
	    *enhanced_next++ = (*base + 3 * *(base_next) + 2) >> 2;

	    enhanced = enhanced + (horiz << 1);
	    enhanced_next = enhanced + (horiz << 1);
	    base++;
	    base_next++;
	}

	/* Bottom left corner pel */
	*enhanced++ = *base;
	/* Last row */
	for (i = 1; i < horiz; i++) {
	    *enhanced++ = (3 * *base + *(base + 1) + 2) >> 2;
	    *enhanced++ = (*base + 3 * *(base + 1) + 2) >> 2;
	    base++;
	}
	/* Bottom right corner pel */
	*enhanced = *base;

	break;

    default:

	break;
    }
}

int
FrameLayerRC(Pict * pic)
{
    int             frameskip = 0; ///SV-XXX: initialised

    switch (rate_control_method) {
    case NO:
	frameskip = chosen_frameskip;
	break;

    case TMN5_RC:
    case TMN8_RC:
	CommBacklog = mmax(0, CommBacklog + (int) (bits->total - (float) pic->bit_rate /
			      ((pic->PB) ? (pic->target_frame_rate / 2) :
			       (pic->target_frame_rate))));
	frameskip = 1;
	while (CommBacklog > ((float) pic->bit_rate / pic->target_frame_rate)) {
	    CommBacklog = mmax(0, (int) (CommBacklog - (float) pic->bit_rate /
					 pic->target_frame_rate));
	    frameskip += 1;
	}
	frameskip = (int) (frameskip * pic->src_frame_rate / pic->target_frame_rate);
	break;

    case OFFLINE_RC:

	/* Aim for the targetrate with a once per frame rate control scheme */
	if (frame_no - start > (end - start) * start_rate_control / 100.0)
	    /* when generating the MPEG-4 anchors, rate control was started
	       after 70% of the sequence was finished.
	       Set start_rate_control with option "-R <n>" */

	    pic->QUANT = FrameUpdateQP(total_bits->total + intra_bits->total,
				       bits->total,
	       (end - frame_no) / chosen_frameskip + ((pic->PB) ? 1 : 0),
				       QP, targetrate, seconds);
	frameskip = chosen_frameskip;
	break;
    default:
	frameskip = chosen_frameskip; //SV-XXX: Debian
	break;

    }

    if (frameskip > 256) {
	fprintf(stderr, "Warning: frameskip > 256\n");
    }
    return frameskip;

}
