/************************************************************************
 *
 *  tmndec.c, main(), initialization, options for tmndecode (H.263 decoder)
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
 * based on mpeg2decode, (C) 1994, MPEG Software Simulation Group and
 * mpeg2play, (C) 1994 Stefan Eckart <stefan@lis.e-technik.tu-muenchen.de>
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#define GLOBAL
#include "config.h"
#include "tmndec.h"
#include "global.h"

#ifdef USE_TIME
#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif
#endif
#ifdef WIN32
#include <io.h>
#endif



#ifdef WINDOWS
int initDisplay (int pels, int lines);
int closeDisplay ();
#endif

/* private prototypes */
static void initdecoder _ANSI_ARGS_ ((void));
static void options _ANSI_ARGS_ ((int *argcp, char **argvp[]));
static int getval _ANSI_ARGS_ ((char *argv[]));
static void flushdisplaybuffer _ANSI_ARGS_ ((int framenum));
/* private data */
static int loopflag;

#ifdef VIC

#include "libh263.h"

extern char *vic_packetbuf;

/* the decoder is'nt reentrant -- therefore I copy around the important
   state information */

static struct CONF {
    char *ptr;
    int   size;
} config[] = {
    { (char*)(&next_I_P_frame), sizeof(char*)*3 },
    { (char*)(&prev_I_P_frame), sizeof(char*)*3 },
    { (char*)(&current_frame), sizeof(char*)*3 },
    { (char*)(&prev_frame), sizeof(char*)*3 },
    { (char*)(&bframe), sizeof(char*)*3 },
    { (char*)(&tmp_f), sizeof(char*)*3 },
    { (char*)(&tmp_b), sizeof(char*)*3 },
    { (char*)(&buffered_frame), sizeof(char*)*3 },
    { (char*)(&edgeframeorig), sizeof(char*)*3 },
    { (char*)(&nextedgeframeorig), sizeof(char*)*3 },
    { (char*)(&ring_lum), sizeof(char*)*RING_SIZE },
    { (char*)(&ring_c1), sizeof(char*)*RING_SIZE },
    { (char*)(&ring_c2), sizeof(char*)*RING_SIZE },
    { (char*)(&ring_tr), sizeof(int)*RING_SIZE },
    { (char*)(&ring_qu), sizeof(int)*RING_SIZE },

    { (char*)(&ring_ptr), sizeof(int) },
    { (char*)(&coded_picture_width), sizeof(int) },
    { (char*)(&coded_picture_height), sizeof(int) },
    { (char*)(&reference_picture_selection_mode), sizeof(int) },
    { (char*)(&stop_decoder), sizeof(int) },
    { NULL }
};

static int current_conf;

static void save_conf(char *dest, struct CONF *conf)
{
    int i,n;
    for (i = 0, n = 0; conf[i].ptr != NULL; n += conf[i].size, i++)
	memcpy(&dest[n],conf[i].ptr,conf[i].size);
}

static void restore_conf(char *src, struct CONF *conf)
{
    int i,n;
    for (i = 0, n = 0; conf[i].ptr != NULL; n += conf[i].size, i++)
	memcpy(conf[i].ptr,&src[n],conf[i].size);
}

struct H263_STATE*
h263_start()
{
    static int count = 0;
    struct H263_STATE *state;

    state = malloc(sizeof(struct H263_STATE));
    memset(state,0,sizeof(struct H263_STATE));
    state->conf = ++count;
    return state;
}

void
h263_stop(struct H263_STATE *state)
{
    int cc;

    if (state->conf != current_conf)
	restore_conf(state->conf_data,config);
    current_conf = state->conf;

    for (cc = 0; cc < 3; cc++) {
	free(next_I_P_frame[cc]);
	free(prev_I_P_frame[cc]);
	free(bframe[cc]);
	free(tmp_f[cc]);
	free(tmp_b[cc]);
	free(buffered_frame[cc]);
	free(edgeframeorig[cc]);
	free(nextedgeframeorig[cc]);
    }
    free(state);
}

int
h263_decode_frame(struct H263_STATE *state, char *packet)
{
    if (0 == state->framenum) {
	initoptions();
	temp_ref = 0;
	prev_non_disposable_temp_ref = next_non_disposable_temp_ref = 0;
	ld = &base;
	quiet = 0;
	outtype = -1; /* no file output! */
	outputname = "";
	stop_decoder = 1;
	memset(ring_lum,0,sizeof(ring_lum));
    } else if (state->conf != current_conf)
	restore_conf(state->conf_data,config);
    current_conf = state->conf;

    vic_packetbuf = packet;
    initbits ();
    
    getheader();
    if (pict_type == PCT_INTRA)
	stop_decoder = 0;
    if (0 == state->framenum) {
        base_source_format = source_format;
        initdecoder ();
	if (stop_decoder) {
	    getpicture (&state->framenum);
	    state->framenum++;
	    save_conf(state->conf_data, config);
	    fprintf(stderr,"decoder start -- waiting for I-frame\n");
	    return -1;
	}
    }
    if (stop_decoder)
	return -1;
    getpicture (&state->framenum);
    state->framenum++;
    save_conf(state->conf_data, config);

    state->frame  = buffered_frame;
    state->width  = coded_picture_width;
    state->height = coded_picture_height;
    return 0;
}

#else /* VIC */

int main (int argc, char *argv[])
{
  int first, framenum;
#ifdef USE_TIME
  int runtime;
#ifndef WIN32
  struct timeval tstart, tstop;
#else
  unsigned int startTime, stopTime;
#endif
#endif

#ifdef USE_TIME
  /* default is read frame rate from bitstream */
  framerate = 99;
#endif

  options (&argc, &argv);

  if (trace)
  {
    if ((trace_file = fopen (trace_file_name, "wt")) == NULL)
    {
      printf ("Unable to open trace file");
      exit (-1);
    }
  }
  if (save_frames)
  {
    if ((recon_file_ptr = fopen (recon_file_name, "wb")) == NULL)
    {
      printf ("Unable to open reconstruction file");
      exit (-1);
    }
  }
  /* pointer to name of output files */
#if (defined DISPLAY || defined WINDOWS)
  if (outtype == T_X11 || outtype == T_WIN)
    outputname = "";
  else
#endif
    outputname = argv[argc - 1];

  ld = &base;

  /* open MPEG input file(s) */
  if (0 == strcmp(argv[1],"stdin"))
      base.infile = 0;
  else if ((base.infile = open (argv[1], O_RDONLY | O_BINARY)) < 0)
  {
    sprintf (errortext, "Input file %s not found\n", argv[1]);
    error (errortext);
  }
  first = 1;

  do
  {
    if (base.infile != 0)
      lseek (base.infile, 0l, 0);
    initbits ();
    initoptions ();
    framenum = 0;
    temp_ref = 0;
    prev_non_disposable_temp_ref = next_non_disposable_temp_ref = 0;

    while (getheader ())
    {
      if (first)
      {  
        base_source_format = source_format;
        initdecoder ();
#ifdef USE_TIME
#ifndef WIN32
        gettimeofday (&tstart, (struct timezone *) NULL);
        if (framerate > 0)
          gettimeofday (&tftarget, (struct timezone *) NULL);
#else
        startTime = timeGetTime ();
        if (framerate > 0)
          targetTime = timeGetTime ();
#endif
#endif
        first = 0;
      }
      getpicture (&framenum);

#if 0 /* for debugging */
      if (trace)
	  fflush(trace_file);
      getchar();
#endif

      framenum++;
    }

    flushdisplaybuffer (framenum);

  } while (loopflag);

  close (base.infile);

#ifdef USE_TIME
#ifndef WIN32
  gettimeofday (&tstop, (struct timezone *) NULL);
  runtime = 100 * (tstop.tv_sec - tstart.tv_sec)
    + (tstop.tv_usec - tstart.tv_usec) / 10000;
#else
  stopTime = timeGetTime ();
  runtime = (stopTime - startTime) / 10;
#endif
  if (!quiet && runtime != 0)
    printf ("%d.%02d seconds, %d frames, %d.%02d fps\n",
            runtime / 100, runtime % 100,
            framenum, ((10000 * framenum + runtime / 2) / runtime) / 100,
            ((10000 * framenum + runtime / 2) / runtime) % 100);
#endif

#ifdef DISPLAY
  if (outtype == T_X11)
    exit_display ();
#endif
#ifdef WINDOWS
  if (outtype == T_WIN)
    closeDisplay ();
#endif

  return 0;
}
#endif /* VIC */

static void initdecoder ()
{
  int i, cc, size;
  FILE *cleared;

  /* clip table */
  if (!(clp = (unsigned char *) malloc (1024)))
    error ("malloc failed\n");

  clp += 384;

  for (i = -384; i < 640; i++)
    clp[i] = (i < 0) ? 0 : ((i > 255) ? 255 : i);

  /* MPEG-1 = TMN parameters */
  matrix_coefficients = 5;

  blk_cnt = 6;

  for (i=0; i<MAX_LAYERS; i++)
    enhancement_layer_init[i] = OFF;

  for (cc = 0; cc < 3; cc++)
  {
    if (cc == 0)
      size = coded_picture_width * coded_picture_height;
    else
      size = chrom_width * chrom_height;

    /* Used for bidirectional and direct prediction mode for true B
     * pictures, one for forward and one for backward. */
    if (!(next_I_P_frame[cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");

    if (!(prev_I_P_frame[cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");

#if 1
    if (!(current_frame[cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");
#endif

    if (!(bframe[cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");
    
    prev_frame[cc] = current_frame[cc];
    
    if (!(tmp_f[cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");

    if (!(tmp_b[cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");

    /* Stores frame in display buffer */
    if (!(buffered_frame[cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");
  }

  for (cc = 0; cc < 3; cc++)
  {
    if (cc == 0)
    {
      size = (coded_picture_width + 64) * (coded_picture_height + 64);

      /* Stores framed version of previous inter-picture, luminance. */
      if (!(edgeframeorig[cc] = (unsigned char *) malloc (size)))
        error ("malloc failed\n");
      edgeframe[cc] = edgeframeorig[cc] + (coded_picture_width + 64) * 32 + 32;

      /* Required for true B frames backward, bidir, and direct
       * prediction. Stores future inter-picture, luminance */
      if (!(nextedgeframeorig[cc] = (unsigned char *) malloc (size)))
        error ("malloc failed\n");
      nextedgeframe[cc] = nextedgeframeorig[cc] + (coded_picture_width + 64) * 32 + 32;
    } 
    else
    {
      size = (chrom_width + 32) * (chrom_height + 32);

      /* Stores framed version of previous inter-picture, chrominance. */
      if (!(edgeframeorig[cc] = (unsigned char *) malloc (size)))
        error ("malloc failed\n");
      edgeframe[cc] = edgeframeorig[cc] + (chrom_width + 32) * 16 + 16;

      /* Required for true B frames backward, bidir, and direct
       * prediction. Stores future inter-picture, chrominance. */
      if (!(nextedgeframeorig[cc] = (unsigned char *) malloc (size)))
        error ("malloc failed\n");
      nextedgeframe[cc] = nextedgeframeorig[cc] + (chrom_width + 32) * 16 + 16;
    }
  }

  if (expand)
  {
    for (cc = 0; cc < 3; cc++)
    {
      if (cc == 0)
        size = coded_picture_width * coded_picture_height * 4;
      else
        size = chrom_width * chrom_height * 4;

      if (!(exnewframe[cc] = (unsigned char *) malloc (size)))
        error ("malloc failed\n");
    }
  }
  /* Clear output file for concatenated storing */
  if (outtype == T_YUV_CONC)
  {
    if ((cleared = fopen (outputname, "wb")) == NULL)
      error ("couldn't clear outputfile\n");
    else
      fclose (cleared);
  }
  /* IDCT */
  if (refidct)
    init_idctref ();
  else
    init_idct ();

#ifdef DISPLAY
  if (outtype == T_X11)
  {
    init_display ("");
  }
#endif
#ifdef WINDOWS
  if (outtype == T_WIN)
  {
    initDisplay (coded_picture_width, coded_picture_height);
  }
#endif


}

void error (char *text)
{
  fprintf (stderr, text);
  exit (1);
}

/* trace output */
void printbits (int code, int bits, int len)
{
  int i;
  for (i = 0; i < len; i++)
    fprintf (trace_file, "%d", (code >> (bits - 1 - i)) & 1);
}

#ifndef VIC

/* option processing */
static void options (int *argcp, char **argvp[])
{
  trace = 0;
  save_frames = 0;

  while (*argcp > 1 && (*argvp)[1][0] == '-')
  {
    while ((*argvp)[1][1])
    {
      switch (toupper ((*argvp)[1][1]))
      {
#ifdef USE_TIME
        case 'F':
          framerate = getval (*argvp);
          break;
#endif
        case 'V':
          verbose = getval (*argvp);
          break;
        case 'O':
          outtype = getval (*argvp);
          break;
        case 'R':
          refidct = 1;
          break;
        case 'L':
          loopflag = 1;
          break;
        case 'X':
          expand = 1;
          break;
        case 'T':
          trace = 1;
          strcpy (trace_file_name, "trace.dec");
          break;
        case 'S':
          save_frames = 1;
          (*argvp)++;
          (*argcp)--;
          strcpy (recon_file_name, (*argvp)[1]);
          (*argvp)[1] += strlen (recon_file_name);
          if (recon_file_name[0] == '-')
          {
            printf ("Reconstruction file name is expected after -S option");
            exit (-1);
          }
          break;

        case 'Q':
          quiet = 1;
          break;
        case 'P':
          post_filter = 1;
          break;
        
        default:
          fprintf (stderr, "undefined option -%c ignored\n", (*argvp)[1][1]);
      }

      (*argvp)[1]++;
    }

    (*argvp)++;
    (*argcp)--;
  }


  if (outtype != T_X11 && outtype != T_WIN)
  {
    loopflag = 0;               /* No looping for output to file */
#ifdef USE_TIME
    framerate = 0;              /* No delay necessary when output to file */
#endif
  }
#ifdef DISPLAY
  if (outtype == T_X11)
  {
    (*argcp)++;                 /* fake outfile parameter */
  }
#endif
#ifdef WINDOWS
  if (outtype == T_WIN)
  {
    (*argcp)++;                 /* fake outfile parameter */
  }
#endif

  if (*argcp != 3 && *argcp != 4)
  {
    printf ("\n%s\n", version);
    printf ("Usage:   tmndecode {options} bitstream {outputfilename%%d}\n\
Options: -vn  verbose output (n: level)\n\
         -on  output format \n\
              n=0 : YUV\n\
              n=1 : SIF\n\
              n=2 : TGA\n\
              n=3 : PPM\n");
#ifdef DISPLAY
    printf ("\
              n=4 : X11 Display\n");
#endif
    printf ("\
              n=5 : YUV concatenated\n");
#ifdef WINDOWS
    printf ("\
              n=6 : Windows 95/NT Display\n");
#endif
    printf ("\
              You have to choose one output format!\n\
         -q   disable warnings to stderr\n\
         -r   use double precision reference IDCT\n\
         -t   enable low level tracing\n\
         -s <filename>  output reconstructed frame to filename (YUV concatenated)\n\
         -p   enable tmn-8 post filter\n");
#ifdef DISPLAY
    printf ("\
         -x   interpolate pictures to double size before display\n");
#endif
#ifdef USE_TIME
    printf ("\
         -fn  frame rate\n\
              n=0  : as fast as possible\n\
              n=99 : read frame rate from bitstream (default)\n");
#endif
#ifdef DISPLAY
    printf ("\
         -l   loop sequence\n");
#endif
    exit (0);
  }
}

#endif /* VIC */

static int getval (char *argv[])
{
  int val;

  if (sscanf (argv[1] + 2, "%d", &val) != 1)
    return 0;

  while (isdigit (argv[1][2]))
    argv[1]++;

  return val;
}



#ifdef USE_TIME
#ifndef WINDOWS

/* Unix version */
void doframerate (int pb)
{
  struct timeval tfdiff;
  const float REF_FRAME_RATE = 29.97;

  /* Compute desired frame rate */
  if (framerate <= 0)
    return;

  if (framerate != 99)
  {
    tftarget.tv_usec += 1000000 / framerate;
  } else
  {
    if (pb)
    {
      tftarget.tv_usec += 1000000 / (REF_FRAME_RATE / trb);
    } else
    {
      tftarget.tv_usec += 1000000 / (REF_FRAME_RATE / (trd - trb));
    }
  }

  /* this is where we should be */
  if (tftarget.tv_usec >= 1000000)
  {
    tftarget.tv_usec -= 1000000;
    tftarget.tv_sec++;
  }
  /* this is where we are */
  gettimeofday (&tfdiff, (struct timezone *) NULL);

  tfdiff.tv_usec = tftarget.tv_usec - tfdiff.tv_usec;
  tfdiff.tv_sec = tftarget.tv_sec - tfdiff.tv_sec;
  if (tfdiff.tv_usec < 0)
  {
    tfdiff.tv_usec += 1000000;
    tfdiff.tv_sec--;
  }
  /* See if we are already lagging behind */
  if (tfdiff.tv_sec < 0 || (tfdiff.tv_sec == 0 && tfdiff.tv_usec <= 0))
    return;

  /* Spin for awhile */
  select (0, NULL, NULL, NULL, &tfdiff);
}

#else

/* Win32 version */
void doframerate (int pb)
{
  DWORD currentTime;
  int diffTime;
  const float REF_FRAME_RATE = (float) 29.97;

  /* Compute desired frame rate */
  if (framerate <= 0)
    return;

  if (framerate != 99)
  {
    targetTime += 1000 / framerate;
  } else
  {
    if (pb)
    {
      targetTime += (int) (1000 / (REF_FRAME_RATE / trb));
    } else
    {
      targetTime += (int) (1000 / (REF_FRAME_RATE / (trd - trb)));
    }
  }

  /* this is where we are */
  currentTime = timeGetTime ();


  diffTime = targetTime - currentTime;

  /* See if we are already lagging behind */
  if (diffTime <= 0)
    return;

  /* Spin for awhile */
  Sleep (diffTime);
  /* this is not a very accurate timer */
}

#endif

#endif

/**********************************************************************
 *
 *	Name:		      flushdisplaybuffer
 *	Description:  empties last frame from display buffer when sequence 
 *                fully decoded
 *	Input:	      frame number
 *	Returns:      
 *	Side effects: 
 *
 *  Date: 971102  Author: mikeg@ee.ubc.ca
 *
 ***********************************************************************/

void flushdisplaybuffer (int framenum)
{

  /* Ensure the dimensions are set properly for displaying the last picture in 
   * the display buffer */
  coded_picture_width = horizontal_size = lines[base_source_format];
  coded_picture_height = vertical_size = pels[base_source_format];

  mb_width = horizontal_size / 16;
  mb_height = vertical_size / 16;
  chrom_width = coded_picture_width >> 1;
  chrom_height = coded_picture_height >> 1;

  if (expand && outtype == T_X11)
  {
    /* display this image and keep frame already in storage where it is. */
    interpolate_image (buffered_frame[0], exnewframe[0],
                       coded_picture_width, coded_picture_height);
    interpolate_image (buffered_frame[1], exnewframe[1], chrom_width, chrom_height);
    interpolate_image (buffered_frame[2], exnewframe[2], chrom_width, chrom_height);

    storeframe (exnewframe, framenum);
    if (save_frames)
    {
      save_frame (exnewframe, framenum, recon_file_ptr);
    }
  } else
  {
    storeframe (buffered_frame, framenum);
    if (save_frames)
    {
      save_frame (buffered_frame, framenum, recon_file_ptr);
    }
  }
}
