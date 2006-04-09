/*****************************************************************
 *
 *  io.c, part of tmn (TMN encoder)
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
 *  Copyright (C) 1995, 1996  Telenor R&D, Norway
 *
 *  Contacts:
 *  Robert Danielsen                  <Robert.Danielsen@nta.no>
 *
 *  Telenor Research and Development  http://www.nta.no/brukere/DVC/
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
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

#include        "sim.h"

//SV-XXX: defined UNUSED() macro for unused variables
#ifndef UNUSED
#define UNUSED(x) (x) = (x)
#endif

#ifdef VIC
/*extern */
unsigned char *h263_frame;
#endif

/**********************************************************************
 *
 *	Name:		ReadImage
 *	Description:	Reads one qcif image from disk
 *
 *	Input:		filename of sequence, frame no. to be read,
 *			headerlength of sequence
 *	Returns:	Pointer to start of raw YUV-data
 *	Side effects:	Memory allocated to image-data
 *
 *	Date: 940108	Author:	Karl.Lillevold@nta.no
 *
 *      Modified:       guyc@ee.ubc.ca 8/97 to read yuv files if necessary
 *
 ***********************************************************************/

unsigned char *
 ReadImage (char *filename, int frame_no, int headerlength)
{
#ifdef VIC
  //SV-XXX: dummy initialisations to make gcc-4.0 shut up
  UNUSED(filename);
  UNUSED(frame_no);
  UNUSED(headerlength);

  return h263_frame;
#else
  FILE *im_file = NULL;
  int im_size = sed_pels * sed_lines * 3 / 2;
  unsigned char *qcif;
  int status;
  char YUVname[255], tmp[255];
  FILE *YUVfile;

  if ((qcif = (unsigned char *) malloc (sizeof (char) * im_size)) == NULL)
  {
    fprintf (stderr, "Couldn't allocate memory to image\n");
    exit (-1);
  }
  if ((im_file = fopen (filename, "rb")) == NULL)
  {

    /* Try using separate file names (yuv files) */
    sprintf (YUVname, filename, frame_no);
    strcpy (tmp, YUVname);
    sprintf (YUVname, "%s.yuv", tmp);

    if ((YUVfile = fopen (YUVname, "rb")) == NULL)
    {
      fprintf (stderr, "Unable to open image_file yuv: %s\n", YUVname);
      exit (-1);
    }
    fprintf (stdout, "Reading image no: %d\n", frame_no);
    if ((status = fread (qcif, sizeof (char), im_size, YUVfile)) != im_size)
    {
      fclose (YUVfile);
      fprintf (stderr, "Error in reading yuv image from %s \n", YUVname);
      exit (-1);
    }
    fclose (YUVfile);

  } else
  {

    rewind (im_file);
    /* Find the correct image */
    status = fseek (im_file, (headerlength + frame_no * im_size), 0);
    if (status != 0)
    {
      fprintf (stderr, "Error in seeking image no: %d\n", frame_no);
      fprintf (stderr, "From file: %s\n", filename);
      exit (-1);
    }
    /* Read image */
    fprintf (stdout, "Reading image no: %d\n", frame_no);
    if ((status = fread (qcif, sizeof (char), im_size, im_file)) != im_size)
    {
      fprintf (stderr, "Error in reading image no: %d\n", frame_no);
      fprintf (stderr, "From file: %s\n", filename);
      exit (-1);
    }
    fclose (im_file);
  }
  return (qcif);
#endif
}


/**********************************************************************
 *
 *	Name:		FillImage
 *	Description:	fills Y, Cb and Cr of a PictImage struct
 *
 *	Input:		pointer to raw image
 *
 *	Returns:	pointer to filled PictImage
 *	Side effects:	allocates memory to PictImage
 *                      raw image is freed
 *
 *	Date: 940109	Author:	Karl.Lillevold@nta.no
 *
 ***********************************************************************/

PictImage *
 FillImage (unsigned char *in)
{
  PictImage *Pict;

  Pict = InitImage (sed_pels * sed_lines);

  memcpy (Pict->lum, in, sed_pels * sed_lines);
  memcpy (Pict->Cb, (in + sed_pels * sed_lines), sed_pels * sed_lines / 4);
  memcpy (Pict->Cr, (in + sed_pels * sed_lines + sed_pels * sed_lines / 4), sed_pels * sed_lines / 4);

#ifdef VIC
  if (in != h263_frame)
#endif
  free (in);
  return (Pict);
}


/**********************************************************************
 *
 *	Name:		WriteImage
 *	Description:	Writes PictImage struct to disk
 *
 *	Input:		pointer to image data to be stored, filename
 *			to be used on the disk, image size
 *	Returns:
 *	Side effects:
 *
 *	Date: 930115	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void
 WriteImage (PictImage * image, char *filename)
{
  UNUSED(image);
  UNUSED(filename);
#ifndef VIC
  int status;
  FILE *f_out;

  /* Opening file */
  if ((f_out = fopen (filename, "ab")) == NULL)
  {
    fprintf (stderr, "%s%s\n", "Error in opening file: ", filename);
    exit (-1);
  }
  /* Writing lum to file */
  if ((status = fwrite (image->lum, sizeof (char), sed_pels * sed_lines, f_out)) != sed_pels * sed_lines)
  {
    fprintf (stderr, "%s%s\n", "Error in writing to file: ", filename);
    exit (-1);
  }
  /* Writing Cb to file */
  if ((status = fwrite (image->Cb, sizeof (char), sed_pels * sed_lines / 4, f_out)) != sed_pels * sed_lines / 4)
  {
    fprintf (stderr, "%s%s\n", "Error in writing to file: ", filename);
    exit (-1);
  }
  /* Writing Cr to file */
  if ((status = fwrite (image->Cr, sizeof (char), sed_pels * sed_lines / 4, f_out)) != sed_pels * sed_lines / 4)
  {
    fprintf (stderr, "%s%s\n", "Error in writing to file: ", filename);
    exit (-1);
  }
  fclose (f_out);
#endif
  return;
}


/**********************************************************************
 *
 *	Name:		InitImage
 *	Description:	Allocates memory for structure of 4:2:0-image
 *
 *	Input:	        image size
 *	Returns:	pointer to new structure
 *	Side effects:	memory allocated to structure
 *
 *	Date: 930115  	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

PictImage *
 InitImage (int size)
{
  PictImage *new;

  if ((new = (PictImage *) malloc (sizeof (PictImage))) == NULL)
  {
    fprintf (stderr, "Couldn't allocate (PictImage *)\n");
    exit (-1);
  }
  if ((new->lum = (unsigned char *) malloc (sizeof (char) * size)) == NULL)
  {
    fprintf (stderr, "Couldn't allocate memory for luminance\n");
    exit (-1);
  }
  if ((new->Cr = (unsigned char *) malloc (sizeof (char) * size / 4)) == NULL)
  {
    fprintf (stderr, "Couldn't allocate memory for Cr\n");
    exit (-1);
  }
  if ((new->Cb = (unsigned char *) malloc (sizeof (char) * size / 4)) == NULL)
  {
    fprintf (stderr, "Couldn't allocate memory for Cb\n");
    exit (-1);
  }
  return (new);
}


/**********************************************************************
 *
 *	Name:		FreeImage
 *	Description:	Frees memory allocated to structure of 4:2:0-image
 *
 *	Input:		pointer to structure
 *	Returns:
 *	Side effects:	memory of structure freed
 *
 *	Date: 930115	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void
 FreeImage (PictImage * image)
{
  free (image->lum);
  free (image->Cr);
  free (image->Cb);
  free (image);
}

/**********************************************************************
 *
 *	Name:		StripName
 *	Description:	Removes characters behind '.' and in front of
 *			(including) the last '/'.
 *
 *	Input:		pointer to character string
 *	Returns:	pointer to new character string
 *	Side effects:
 *
 *	Date:	1992	Author:	Karl.Lillevold@nta.no
 *
 ***********************************************************************/


char *
 StripName (char *s)
{
  char *t;
  char *tmp;
  int counter = 0;
  int mark = 0;

  tmp = (char *) malloc (sizeof (char) * 60);
  strcpy (tmp, s);
  t = tmp;
  while ((*t != '\0') && (*t != '.') && (*t != ' '))
  {
    counter++;
    if (*t == '/')
      mark = counter;
    t++;
  }
  *t = '\0';
  strncpy (tmp, (tmp + mark), (counter - mark + 1));

  return (tmp);
}
