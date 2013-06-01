/*
 * yuv_convert.cpp --
 *
 *      Defines generic yuv_conversion routines
 *
 *      --destWidth and destHeight specify image dimensions for dest buffer
 *        srcWidth and srcHeight specify image dimensions for src buffer
 *        dimensions are specified in terms of size of video image
 *      --horizonal dimensions should be a multiple of 16, although 8
 *        is acceptable for certain video formats
 *
 * Copyright (c) 2001 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "yuv_convert.h"
#include <string.h>
#include <stdio.h>

#include "bsd-endian.h"

//
//  planarYUYV422_to_planarYUYV422
//
//  This function (in the simple case) does a memory copy, but it also
//  can adjust the image dimensions
//
bool planarYUYV422_to_planarYUYV422(char *dest, int destWidth, int destHeight,
				    const char *src, int srcWidth, int srcHeight)
{
  char *srca = (char *)src;

  if ((destWidth & 0x1) || (srcWidth & 0x1)) {
    printf ("even width required in planarYUYV422_to_planarYUYV422\n");
    return false;
  }
  if (destHeight != srcHeight || destWidth != srcWidth) {
    int leftPad = (srcWidth < destWidth) ? ((destWidth - srcWidth) >> 1) : 0;
    int leftClip = (srcWidth > destWidth) ? ((srcWidth - destWidth) >> 1) : 0;
    int rightPad =
      (srcWidth < destWidth) ? (destWidth - srcWidth - leftPad) : 0;
    int rightClip =
      (srcWidth > destWidth) ? (srcWidth - destWidth - leftClip) : 0;

    int upPad = (srcHeight < destHeight) ? ((destHeight - srcHeight) >> 1) : 0;
    int upClip = (srcHeight > destHeight) ? ((srcHeight - destHeight) >> 1) : 0;
    int downPad =
      (srcHeight < destHeight) ? (destHeight - srcHeight - upPad) : 0;
    int downClip =
      (srcHeight > destHeight) ? (srcHeight - destHeight - upClip) : 0;
    int rows = (srcHeight > destHeight) ? destHeight : srcHeight;

    int i;

    // copy the y data

    // handle the up padding on destination
    dest += (destWidth * upPad);
    // handle the up clipping on source
    srca += (srcWidth * upClip);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < rows; ++i) {
	dest += leftPad;
	memcpy(dest, srca, srcWidth);
	dest += srcWidth + rightPad;
	srca += srcWidth;
      }
    } else { // if clipping necessary on source
      for (i = 0; i < rows; ++i) {
	srca += leftClip;
	memcpy(dest, srca, destWidth);
	dest += destWidth;
	srca += destWidth + rightClip;
      }
    }

    // handle the down padding on destination
    dest += (destWidth * downPad);
    // handle the down clipping on source
    srca += (srcWidth * downClip);

    // copy the u data

    // handle the up padding on destination
    dest += ((destWidth * upPad) >> 1);
    // handle the up clipping on source
    srca += ((srcWidth * upClip) >> 1);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < rows; ++i) {
	dest += (leftPad >> 1);
	memcpy(dest, srca, (srcWidth >> 1));
	dest += ((srcWidth + rightPad) >> 1);
	srca += (srcWidth >> 1);
      }
    } else { // if clipping necessary on source
      for (i = 0; i < rows; ++i) {
	srca += (leftClip >> 1);
	memcpy(dest, srca, (destWidth >> 1));
	dest += (destWidth >> 1);
	srca += ((destWidth + rightClip) >> 1);
      }
    }
    // handle the down padding on destination
    dest += ((destWidth * downPad) >> 1);
    // handle the down clipping on source
    srca += ((srcWidth * downClip) >> 1);

    // copy the v data

    // handle the up padding on destination
    dest += ((destWidth * upPad) >> 1);
    // handle the up clipping on source
    srca += ((srcWidth * upClip) >> 1);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < rows; ++i) {
	dest += (leftPad >> 1);
	memcpy(dest, srca, (srcWidth >> 1));
	dest += ((srcWidth + rightPad) >> 1);
	srca += (srcWidth >> 1);
      }
    } else { // if clipping necessary on source
      for (i = 0; i < rows; ++i) {
	srca += (leftClip >> 1);
	memcpy(dest, srca, (destWidth >> 1));
	dest += (destWidth >> 1);
	srca += ((destWidth + rightClip) >> 1);
      }
    }
    // no actions needed for final down padding
  } else { // sizes all the same, so can just copy data
    memcpy(dest, srca, (size_t) ((destHeight * destWidth) << 1));
  }
  return true;
}

//
//  planarYUYV422_to_planarYUYV420
//
// This function downsamples a planar-422 frame to a planar-420 one
bool planarYUYV422_to_planarYUYV420(char *dest, int destWidth, int destHeight,
				    const char *src, int srcWidth, int srcHeight)
{
  char *srca = (char *)src;

  if ((destWidth & 0x1) || (srcWidth & 0x1)) {
    printf("even width required in planarYUYV422_to_planarYUYV420\n");
    return false;
  }
  if ((destHeight & 0x1) || (srcHeight & 0x1)) {
    printf("even height required in planarYUYV422_to_planarYUYV420\n");
    return false;
  }
  if (destWidth != srcWidth || destHeight != srcHeight) {
    int leftPad = (srcWidth < destWidth) ? ((destWidth - srcWidth) >> 1) : 0;
    int leftClip = (srcWidth > destWidth) ? ((srcWidth - destWidth) >> 1) : 0;
    int rightPad =
      (srcWidth < destWidth) ? (destWidth - srcWidth - leftPad) : 0;
    int rightClip =
      (srcWidth > destWidth) ? (srcWidth - destWidth - leftClip) : 0;

    int upPad =
      (srcHeight < destHeight) ? ((destHeight - srcHeight) >> 1) : 0;
    if (upPad & 0x1) {
      --upPad;
    }
    int upClip =
      (srcHeight > destHeight) ? ((srcHeight - destHeight) >> 1) : 0;
    int downPad =
      (srcHeight < destHeight) ? (destHeight - srcHeight - upPad) : 0;
    int downClip =
      (srcHeight > destHeight) ? (srcHeight - destHeight - upClip) : 0;
    int rows = (srcHeight > destHeight) ? destHeight : srcHeight;

    int i;

    // copy the y data

    // handle the y up padding
    dest += (destWidth * upPad);
    // handle the y up clipping
    srca += (srcWidth * upClip);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < rows; ++i) {
	dest += leftPad;
	memcpy(dest, srca, srcWidth);
	dest += srcWidth + rightPad;
	srca += srcWidth;
      }
    } else { // if clipping necessary on source
      for (i = 0; i < rows; ++i) {
	srca += leftClip;
	memcpy(dest, srca, destWidth);
	dest += destWidth;
	srca += destWidth + rightClip;
      }
    }
    // handle the y down padding
    dest += (destWidth * downPad);
    // handle the y down clipping
    srca += (srcWidth * downClip);

    // copy the u data

    // handle the u up padding
    dest += ((destWidth * upPad) >> 2);
    // handle the u up clipping
    srca += ((srcWidth * upClip) >> 1);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < (rows >> 1); i++) {
	dest += (leftPad >> 1);
	for (int j = 0; j < (srcWidth >> 1); ++j) {
	  *dest = *src;
	  dest++;
	  src++;
	}
	dest += (rightPad >> 1);
	srca += (srcWidth >> 1); // skip a row
      }
    } else { // if clipping necessary on source
      for (i = 0; i < (rows >> 1); i++) {
	srca += (leftClip >> 1);
	for (int j = 0; j < (destWidth >> 1); ++j) {
	  *dest = *src;
	  dest++;
	  src++;
	}
	srca += (rightClip >> 1);
	srca += (srcWidth >> 1); // skip a row
      }
    }
    // handle the u down padding
    dest += ((destWidth * downPad) >> 2);
    // handle the u down clipping
    srca += ((srcWidth * downClip) >> 1);

    // copy the v data

    // handle the v up padding
    dest += ((destWidth * upPad) >> 2);
    // handle the v up clipping
    srca += ((srcWidth * upClip) >> 1);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < (rows >> 1); i++) {
	dest += (leftPad >> 1);
	for (int j = 0; j < (srcWidth >> 1); ++j) {
	  *dest = *src;
	  dest++;
	  src++;
	}
	dest += (rightPad >> 1);
	srca += (srcWidth >> 1); // skip a row
      }
    } else { // if clipping necessary on source
      for (i = 0; i < (rows >> 1); i++) {
	srca += (leftClip >> 1);
	for (int j = 0; j < (destWidth >> 1); ++j) {
	  *dest = *src;
	  dest++;
	  src++;
	}
	srca += (rightClip >> 1);
	srca += (srcWidth >> 1); // skip a row
      }
    }
    // no actions needed for final down padding
  } else {
    int i, j;
    char *srcu, *srcv;
    char *dstu, *dstv;
    srcu = srca + destHeight * destWidth;
    srcv = srcu + ((destHeight * destWidth) >> 1);
    dstu = dest + destHeight * destWidth;
    dstv = dstu + ((destHeight * destWidth) >> 2);

    // copy the y
    memcpy(dest, srca, (size_t) destHeight * destWidth);
    // copy the u and v, downsampling by 2
    for (i = (destHeight >> 1); i > 0; --i) {
      // even lines get all the chroma information
      for (j = (destWidth >> 1); j > 0; j--) {
	*(dstu++) = *(srcu++);
	*(dstv++) = *(srcv++);
      }
      // odd lines get no chroma information
      srcu += (destWidth >> 1);
      srcv += (destWidth >> 1);
    }
  }
  return true;
}

/////////////////////////////////////////////////////////////////
//
//  planarYUYV420_to_planarYUYV422
//
//  This function (in the simple case) does a memory copy, but it also
//  can adjust the image dimensions
bool planarYUYV420_to_planarYUYV422(char *dest, int destWidth, int destHeight,
				    const char *src, int srcWidth, int srcHeight)
{
  char *srca = (char *)src;

  if ((destWidth & 0x1) || (srcWidth & 0x1)) {
    printf("even width required in planarYUYV420_to_planarYUYV422\n");
    return false;
  }
  if ((destHeight & 0x1) || (srcHeight & 0x1)) {
    printf("even height required in planarYUYV420_to_planarYUYV422\n");
    return false;
  }
  if (destHeight != srcHeight || destWidth != srcWidth) {
    int leftPad = (srcWidth < destWidth) ? ((destWidth - srcWidth) >> 1) : 0;
    int leftClip = (srcWidth > destWidth) ? ((srcWidth - destWidth) >> 1) : 0;
    int rightPad =
      (srcWidth < destWidth) ? (destWidth - srcWidth - leftPad) : 0;
    int rightClip =
      (srcWidth > destWidth) ? (srcWidth - destWidth - leftClip) : 0;

    int upPad =
      (srcHeight < destHeight) ? ((destHeight - srcHeight) >> 1) : 0;
    int upClip =
      (srcHeight > destHeight) ? ((srcHeight - destHeight) >> 1) : 0;
    int downPad =
      (srcHeight < destHeight) ? (destHeight - srcHeight - upPad) : 0;
    int downClip =
      (srcHeight > destHeight) ? (srcHeight - destHeight - upClip) : 0;
    int rows = (srcHeight > destHeight) ? destHeight : srcHeight;

    int i;

    // copy the y data

    // handle the up padding on destination
    dest += (destWidth * upPad);
    // handle the up clipping on source
    srca += (srcWidth * upClip);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < rows; ++i) {
	dest += leftPad;
	memcpy(dest, srca, srcWidth);
	dest += srcWidth + rightPad;
	srca += srcWidth;
      }
    } else { // if clipping necessary on source
      for (i = 0; i < rows; ++i) {
	srca += leftClip;
	memcpy(dest, srca, destWidth);
	dest += destWidth;
	srca += destWidth + rightClip;
      }
    }

    // handle the down padding on destination
    dest += (destWidth * downPad);
    // handle the down clipping on source
    srca += (srcWidth * downClip);

    // copy the u data

    // handle the up padding on destination
    dest += ((destWidth * upPad) >> 1);
    // handle the up clipping on source
    srca += ((srcWidth * upClip) >> 2);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < (rows << 1); i += 2) {
	// source information is every-other row, so double each line
	dest += (leftPad >> 1);
	memcpy(dest, srca, (srcWidth >> 1));
	dest += ((srcWidth + rightPad) >> 1);

	dest += (leftPad >> 1);
	memcpy(dest, srca, (srcWidth >> 1));
	dest += ((srcWidth + rightPad) >> 1);

	srca += (srcWidth >> 1);
      }
    } else { // if clipping necessary on source
      for (i = 0; i < (rows << 1); i += 2) {
	srca += (leftClip >> 1);

	// source information is every-other row, so double each line
	memcpy(dest, srca, (destWidth >> 1));
	dest += (destWidth >> 1);

	memcpy(dest, srca, (destWidth >> 1));
	dest += (destWidth >> 1);

	srca += ((destWidth + rightClip) >> 1);
      }
    }
    // handle the down padding on destination
    dest += ((destWidth * downPad) >> 1);
    // handle the down clipping on source
    srca += ((srcWidth * downClip) >> 2);

    // copy the v data

    // handle the up padding on destination
    dest += ((destWidth * upPad) >> 1);
    // handle the up clipping on source
    srca += ((srcWidth * upClip) >> 2);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < (rows << 1); i += 2) {
	// source information is every-other row, so double each line
	dest += (leftPad >> 1);
	memcpy(dest, srca, (srcWidth >> 1));
	dest += ((srcWidth + rightPad) >> 1);

	dest += (leftPad >> 1);
	memcpy(dest, srca, (srcWidth >> 1));
	dest += ((srcWidth + rightPad) >> 1);

	srca += (srcWidth >> 1);
      }
    } else { // if clipping necessary on source
      for (i = 0; i < (rows << 1); i += 2) {
	srca += (leftClip >> 1);

	// source information is every-other row, so double each line
	memcpy(dest, srca, (destWidth >> 1));
	dest += (destWidth >> 1);

	memcpy(dest, srca, (destWidth >> 1));
	dest += (destWidth >> 1);

	srca += ((destWidth + rightClip) >> 1);
      }
    }
    // no actions needed for final down padding
  } else { // sizes all the same, so can just copy data
    memcpy(dest, srca, (size_t) (destHeight * destWidth)); // copy y
    dest += destHeight * destWidth;
    srca += destHeight * destWidth;

    // copy u and v information, doubling each row
    for (int i = 0; i < (destHeight << 1); i += 2) {
      memcpy(dest, srca, (size_t) (destWidth >> 1)); // 1st copy to dest
      dest += destWidth >> 1;
      memcpy(dest, srca, (size_t) (destWidth >> 1)); // 2nd copy to dest
      dest += destWidth >> 1;
      srca += destWidth >> 1;
    }
  }
  return true;
}

//
//  planarYUYV420_to_planarYUYV420
//
// This function downsamples a planar-420 frame to a planar-420 one
bool planarYUYV420_to_planarYUYV420(char *dest, int destWidth, int destHeight,
				    const char *src, int srcWidth, int srcHeight)
{
  char *srca = (char *)src;

  if ((destWidth & 0x1) || (srcWidth & 0x1)) {
    printf("even width required in planarYUYV420_to_planarYUYV420\n");
    return false;
  }
  if ((destHeight & 0x1) || (srcHeight & 0x1)) {
    printf("even height required in planarYUYV420_to_planarYUYV420\n");
    return false;
  }
  if (destWidth != srcWidth || destHeight != srcHeight) {
    int leftPad = (srcWidth < destWidth) ? ((destWidth - srcWidth) >> 1) : 0;
    int leftClip = (srcWidth > destWidth) ? ((srcWidth - destWidth) >> 1) : 0;
    int rightPad =
      (srcWidth < destWidth) ? (destWidth - srcWidth - leftPad) : 0;
    int rightClip =
      (srcWidth > destWidth) ? (srcWidth - destWidth - leftClip) : 0;

    int upPad =
      (srcHeight < destHeight) ? ((destHeight - srcHeight) >> 1) : 0;
    if (upPad & 0x1) {
      --upPad;
    }
    int upClip =
      (srcHeight > destHeight) ? ((srcHeight - destHeight) >> 1) : 0;
    int downPad =
      (srcHeight < destHeight) ? (destHeight - srcHeight - upPad) : 0;
    int downClip =
      (srcHeight > destHeight) ? (srcHeight - destHeight - upClip) : 0;
    int rows = (srcHeight > destHeight) ? destHeight : srcHeight;

    int i;

    // copy the y data

    // handle the y up padding
    dest += (destWidth * upPad);
    // handle the y up clipping
    srca += (srcWidth * upClip);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < rows; ++i) {
	dest += leftPad;
	memcpy(dest, srca, srcWidth);
	dest += srcWidth + rightPad;
	srca += srcWidth;
      }
    } else { // if clipping necessary on source
      for (i = 0; i < rows; ++i) {
	srca += leftClip;
	memcpy(dest, srca, destWidth);
	dest += destWidth;
	srca += destWidth + rightClip;
      }
    }
    // handle the y down padding
    dest += (destWidth * downPad);
    // handle the y down clipping
    srca += (srcWidth * downClip);

    // copy the u data

    // handle the u up padding
    dest += ((destWidth * upPad) >> 2);
    // handle the u up clipping
    srca += ((srcWidth * upClip) >> 2);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < (rows >> 1); i++) {
	dest += (leftPad >> 1);

	memcpy(dest, srca, (srcWidth >> 1));
	dest += (srcWidth >> 1);
	srca += (srcWidth >> 1);

	dest += (rightPad >> 1);
      }
    } else { // if clipping necessary on source
      for (i = 0; i < (rows >> 1); i++) {
	srca += (leftClip >> 1);

	memcpy(dest, srca, (destWidth >> 1));
	dest += (destWidth >> 1);
	srca += (destWidth >> 1);

	srca += (rightClip >> 1);
      }
    }
    // handle the u down padding
    dest += ((destWidth * downPad) >> 2);
    // handle the u down clipping
    srca += ((srcWidth * downClip) >> 2);

    // copy the v data

    // handle the v up padding
    dest += ((destWidth * upPad) >> 2);
    // handle the v up clipping
    srca += ((srcWidth * upClip) >> 2);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < (rows >> 1); i++) {
	dest += (leftPad >> 1);

	memcpy(dest, srca, (srcWidth >> 1));
	dest += (srcWidth >> 1);
	srca += (srcWidth >> 1);

	dest += (rightPad >> 1);
      }
    } else { // if clipping necessary on source
      for (i = 0; i < (rows >> 1); i++) {
	srca += (leftClip >> 1);

	memcpy(dest, srca, (destWidth >> 1));
	dest += (destWidth >> 1);
	srca += (destWidth >> 1);

	srca += (rightClip >> 1);
      }
    }
    // no actions needed for final down padding
  } else {
    // copy the y (w*h), u (w*h*0.25), and v (w*h*0.25), for a total of (w*h*1.5)
    memcpy(dest, srca,
	   (size_t) destHeight * destWidth + ((destHeight * destWidth) >> 1));
  }
  return true;
}

/////////////////////////////////////////////////////////////////

//
//  packedYUYV422_to_planarYUYV422
//
// This function unpacks every frame into a planar form, i.e., does
//  YUYV YUYV ... YUYV -> YY ... Y UU ... U VV ... V
bool packedYUYV422_to_planarYUYV422(char *dest, int destWidth, int destHeight,
				    const char *src, int srcWidth, int srcHeight)
{
  if ((srcWidth & 0x1) || (destWidth & 0x1)) {
    printf("even width required in packedYUYV422_to_planarYUYV422\n");
    return false;
  }
  if (destWidth == srcWidth && destHeight == srcHeight) {
    int i;
    char *s, *y, *u, *v;

    i = ((destWidth * destHeight) >> 1);
    s = (char *)src;
    y = dest;
    u = y + destWidth * destHeight;
    v = u + ((destWidth * destHeight) >> 1);

    // packed representation is YUYV YUYV YUYV
    while (i--) {
#if BYTE_ORDER == BIG_ENDIAN
      *(v++) = *(s++);
      *(y++) = *(s++);
      *(u++) = *(s++);
      *(y++) = *(s++);
#else
      *(y++) = *(s++);
      *(u++) = *(s++);
      *(y++) = *(s++);
      *(v++) = *(s++);
#endif
    }
  } else {
    char *s, *y, *u, *v;

    s = (char *)src;
    y = dest;
    u = y + destWidth * destHeight;
    v = u + ((destWidth * destHeight) >> 1);

    int leftPad = (srcWidth < destWidth) ? ((destWidth - srcWidth) >> 1) : 0;
    int leftClip = (srcWidth > destWidth) ? ((srcWidth - destWidth) >> 1) : 0;
    int rightPad =
      (srcWidth < destWidth) ? (destWidth - srcWidth - leftPad) : 0;
    int rightClip =
      (srcWidth > destWidth) ? (srcWidth - destWidth - leftClip) : 0;

    int upPad =
      (srcHeight < destHeight) ? ((destHeight - srcHeight) >> 1) : 0;
    int upClip =
      (srcHeight > destHeight) ? ((srcHeight - destHeight) >> 1) : 0;

    int rows = (srcHeight > destHeight) ? destHeight : srcHeight;

    // handle the y up padding
    y += (destWidth * upPad);
    u += ((destWidth * upPad) >> 1);
    v += ((destWidth * upPad) >> 1);

    // handle the up clipping
    s += ((srcWidth * upClip) << 1);

    // copy the video
    int i;
    if (leftPad != 0 || rightPad != 0) { // if padding necessary on destination
      for (i = 0; i < rows; ++i) {
	y += leftPad;
	u += (leftPad >> 1);
	v += (leftPad >> 1);

	int j = (srcWidth >> 1);
	// packed representation is YUYV YUYV YUYV
	while (j--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(v++) = *(s++);
	  *(y++) = *(s++);
	  *(u++) = *(s++);
	  *(y++) = *(s++);
#else
	  *(y++) = *(s++);
	  *(u++) = *(s++);
	  *(y++) = *(s++);
	  *(v++) = *(s++);
#endif
	}
	y += rightPad;
	u += (rightPad >> 1);
	v += (rightPad >> 1);
      }
    } else { // if clipping necessary on source
      for (i = 0; i < rows; ++i) {
	s += (leftClip << 1);

	int j = (destWidth >> 1);
	// packed representation is YUYV YUYV YUYV
	while (j--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(v++) = *(s++);
	  *(y++) = *(s++);
	  *(u++) = *(s++);
	  *(y++) = *(s++);
#else
	  *(y++) = *(s++);
	  *(u++) = *(s++);
	  *(y++) = *(s++);
	  *(v++) = *(s++);
#endif
	}
	s += (rightClip << 1);
      }
    }
    // don't perform any action for the padding/clipping on bottom of image
  }
  return true;
}

//
//  packedUYVY422_to_planarYUYV422
//
// This function unpacks every frame into a planar form, i.e., does
//  UYVY UYVY ... YUYV -> YY ... Y UU ... U VV ... V
bool packedUYVY422_to_planarYUYV422(char *dest, int destWidth, int destHeight,
				    const char *src, int srcWidth, int srcHeight)
{
  if ((destWidth & 0x1) || (srcWidth & 0x1)) {
    printf("even width required in packedUYVY422_to_planarYUYV422\n");
    return false;
  }

  if (destWidth == srcWidth && destHeight == srcHeight) {
    int i;
    char *s, *y, *u, *v;

    i = ((destWidth * destHeight) >> 1);
    s = (char *)src;
    y = dest;
    u = y + destWidth * destHeight;
    v = u + ((destWidth * destHeight) >> 1);

    // packed representation is UYVY UYVY UYVY
    while (i--) {
#if BYTE_ORDER == BIG_ENDIAN
      *(y++) = *(s++);
      *(v++) = *(s++);
      *(y++) = *(s++);
      *(u++) = *(s++);
#else
      *(u++) = *(s++);
      *(y++) = *(s++);
      *(v++) = *(s++);
      *(y++) = *(s++);
#endif
    }
  } else { // non-matching size case
    char *s, *y, *u, *v;

    s = (char *)src;
    y = dest;
    u = y + destWidth * destHeight;
    v = u + ((destWidth * destHeight) >> 1);

    int leftPad = (srcWidth < destWidth) ? ((destWidth - srcWidth) >> 1) : 0;
    int leftClip = (srcWidth > destWidth) ? ((srcWidth - destWidth) >> 1) : 0;
    int rightPad =
      (srcWidth < destWidth) ? (destWidth - srcWidth - leftPad) : 0;
    int rightClip =
      (srcWidth > destWidth) ? (srcWidth - destWidth - leftClip) : 0;

    int upPad =
      (srcHeight < destHeight) ? ((destHeight - srcHeight) >> 1) : 0;
    int upClip =
      (srcHeight > destHeight) ? ((srcHeight - destHeight) >> 1) : 0;

    int rows = (srcHeight > destHeight) ? destHeight : srcHeight;

    // handle the y up padding
    y += (destWidth * upPad);
    u += ((destWidth * upPad) >> 1);
    v += ((destWidth * upPad) >> 1);

    // handle the y up clipping
    s += ((srcWidth * upClip) << 1);

    // copy the video
    int i;
    if (leftPad != 0 || rightPad != 0) { // if padding necessary
      for (i = 0; i < rows; ++i) {
	y += leftPad;
	u += (leftPad >> 1);
	v += (leftPad >> 1);

	int j = (srcWidth >> 1);
	// packed representation is YUYV YUYV YUYV
	while (j--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(y++) = *(s++);
	  *(v++) = *(s++);
	  *(y++) = *(s++);
	  *(u++) = *(s++);
#else
	  *(u++) = *(s++);
	  *(y++) = *(s++);
	  *(v++) = *(s++);
	  *(y++) = *(s++);
#endif
	}
	y += rightPad;
	u += (rightPad >> 1);
	v += (rightPad >> 1);
      }
    } else { // if clipping necessary on source
      for (i = 0; i < rows; ++i) {
	s += (leftClip << 1);

	int j = (destWidth >> 1);
	// packed representation is YUYV YUYV YUYV
	while (j--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(y++) = *(s++);
	  *(v++) = *(s++);
	  *(y++) = *(s++);
	  *(u++) = *(s++);
#else
	  *(u++) = *(s++);
	  *(y++) = *(s++);
	  *(v++) = *(s++);
	  *(y++) = *(s++);
#endif
	}
	s += (rightClip << 1);
      }
    }
    // don't perform any action for the padding/clipping on bottom of image
  }
  return true;
}


//
//  packedYUYV422_to_planarYUYV420
//
// This function unpacks every frame into a planar form *and* reduces
//  the color subsampling from 4:2:2 to 4:2:0 by throwing out the
//  chroma information in every other line
bool packedYUYV422_to_planarYUYV420(char *dest, int destWidth, int destHeight,
				    const char *src, int srcWidth, int srcHeight)
{
  if ((destHeight & 0x1) || (srcHeight & 0x1)) {
    printf
      ("even height required in packedYUYV422_to_planarYUYV420\n");
    return false;
  }
  if ((destWidth & 0x1) || (srcWidth & 0x1)) {
    printf("even width required in packedYUYV422_to_planarYUYV420\n");
    return false;
  }

  if (destWidth == srcWidth && destHeight == srcHeight) {

    int a, b;
    char *s, *y, *u, *v;

    s = (char *)src;
    y = dest;
    u = y + destWidth * destHeight;
    v = u + ((destWidth * destHeight) >> 2);

    // packed representation is YUYV YUYV YUYV
    for (a = (destHeight >> 1); a > 0; a--) {
      // The information we have is 4:2:2. The subsampling consists in
      // keeping the chroma info from one line and throwing it out from
      // the next one. This is indeed 4:2:0 subsampling
      for (b = (destWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	*(v++) = *(s++);
	*(y++) = *(s++);
	*(u++) = *(s++);
	*(y++) = *(s++);
#else
	*(y++) = *(s++);
	*(u++) = *(s++);
	*(y++) = *(s++);
	*(v++) = *(s++);
#endif
      }
      for (b = (destWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	s++;
	*(y++) = *(s++);
	s++;
	*(y++) = *(s++);
#else
	*(y++) = *(s++);
	s++;
	*(y++) = *(s++);
	s++;
#endif
      }
    }
  }
  else { // non-matching size case

    int a, b;
    char *s, *y, *u, *v;

    int leftPad = (srcWidth < destWidth) ? ((destWidth - srcWidth) >> 1) : 0;
    int leftClip = (srcWidth > destWidth) ? ((srcWidth - destWidth) >> 1) : 0;
    int rightPad =
      (srcWidth < destWidth) ? (destWidth - srcWidth - leftPad) : 0;
    int rightClip =
      (srcWidth > destWidth) ? (srcWidth - destWidth - leftClip) : 0;

    int upPad =
      (srcHeight < destHeight) ? ((destHeight - srcHeight) >> 1) : 0;
    if (upPad & 0x1) {
      --upPad;
    } // 4:2:0, so deal with even #'ed rows
    int upClip =
      (srcHeight > destHeight) ? ((srcHeight - destHeight) >> 1) : 0;

    int rows = (srcHeight > destHeight) ? destHeight : srcHeight;

    //    int downPad=(srcHeight<destHeight)?(destHeight-srcHeight-upPad):0;

    s = (char *)src;
    y = dest;
    u = y + destWidth * destHeight;
    v = u + ((destWidth * destHeight) >> 2);

    // handle the y up padding
    y += (destWidth * upPad);
    u += ((destWidth * upPad) >> 2);
    v += ((destWidth * upPad) >> 2);

    // handle the y up clipping
    s += ((srcWidth * upClip) << 1);

    if (leftPad != 0 || rightPad != 0) { // if padding necessary
      // packed representation is UYUV UYUV UYUV
      for (a = (rows >> 1); a > 0; a--) {
	y += leftPad;
	u += (leftPad >> 1);
	v += (leftPad >> 1);
	// The information we have is 4:2:2. The subsampling consists in
	// keeping the chroma info from one line and throwing it out from
	// the next one. This is indeed 4:2:0 subsampling
	for (b = (srcWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(v++) = *(s++);
	  *(y++) = *(s++);
	  *(u++) = *(s++);
	  *(y++) = *(s++);
#else
	  *(y++) = *(s++);
	  *(u++) = *(s++);
	  *(y++) = *(s++);
	  *(v++) = *(s++);
#endif
	}
	y += rightPad;
	u += (rightPad >> 1);
	v += (rightPad >> 1);
	y += leftPad;
	for (b = (srcWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	  s++;
	  *(y++) = *(s++);
	  s++;
	  *(y++) = *(s++);
#else
	  *(y++) = *(s++);
	  s++;
	  *(y++) = *(s++);
	  s++;
#endif
	}
	y += rightPad;
      }
    } else { // clipping is necessary
      // packed representation is UYUV UYUV UYUV
      for (a = (rows >> 1); a > 0; a--) {
	s += (leftClip << 1);
	// The information we have is 4:2:2. The subsampling consists in
	// keeping the chroma info from one line and throwing it out from
	// the next one. This is indeed 4:2:0 subsampling
	for (b = (destWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(v++) = *(s++);
	  *(y++) = *(s++);
	  *(u++) = *(s++);
	  *(y++) = *(s++);
#else
	  *(y++) = *(s++);
	  *(u++) = *(s++);
	  *(y++) = *(s++);
	  *(v++) = *(s++);
#endif
	}
	s += (rightClip << 1);
	s += (leftClip << 1);
	for (b = (destWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	  s++;
	  *(y++) = *(s++);
	  s++;
	  *(y++) = *(s++);
#else
	  *(y++) = *(s++);
	  s++;
	  *(y++) = *(s++);
	  s++;
#endif
	}
	s += (rightClip << 1);
      }
    }
  }
  return true;
}

//
//  packedUYVY422_to_planarYUYV420
//
// This function unpacks every frame into a planar form *and* reduces
//  the color subsampling from 4:2:2 to 4:2:0 by throwing out the
//  chroma information in every other line
bool packedUYVY422_to_planarYUYV420(char *dest, int destWidth, int destHeight,
				    const char *src, int srcWidth, int srcHeight)
{
  if ((destHeight & 0x1) || (srcHeight & 0x1)) {
    printf("even height required in packedUYVY422_to_planarYUYV420\n");
    return false;
  }
  if ((destWidth & 0x1) || (srcWidth & 0x1)) {
    printf("even width required in packedUYVY422_to_planarYUYV420\n");
    return false;
  }

  if (destWidth == srcWidth && destHeight == srcHeight) {

    int a, b;
    char *s, *y, *u, *v;

    s = (char*)src;
    y = dest;
    u = y + destWidth * destHeight;
    v = u + ((destWidth * destHeight) >> 2);

    // packed representation is UYUV UYUV UYUV
    for (a = (destHeight >> 1); a > 0; a--) {
      // The information we have is 4:2:2. The subsampling consists in
      // keeping the chroma info from one line and throwing it out from
      // the next one. This is indeed 4:2:0 subsampling
      for (b = (destWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	*(y++) = *(s++);
	*(v++) = *(s++);
	*(y++) = *(s++);
	*(u++) = *(s++);
#else
	*(u++) = *(s++);
	*(y++) = *(s++);
	*(v++) = *(s++);
	*(y++) = *(s++);
#endif
      }
      for (b = (destWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	*(y++) = *(s++);
	s++;
	*(y++) = *(s++);
	s++;
#else
	s++;
	*(y++) = *(s++);
	s++;
	*(y++) = *(s++);
#endif
      }
    }
  } else { // non-matching size case

    int a, b;
    char *s, *y, *u, *v;

    int leftPad = (srcWidth < destWidth) ? ((destWidth - srcWidth) >> 1) : 0;
    int leftClip = (srcWidth > destWidth) ? ((srcWidth - destWidth) >> 1) : 0;
    int rightPad =
      (srcWidth < destWidth) ? (destWidth - srcWidth - leftPad) : 0;
    int rightClip =
      (srcWidth > destWidth) ? (srcWidth - destWidth - leftClip) : 0;

    int upPad =
      (srcHeight < destHeight) ? ((destHeight - srcHeight) >> 1) : 0;
    if (upPad & 0x1) {
      --upPad;
    }
    int upClip =
      (srcHeight > destHeight) ? ((srcHeight - destHeight) >> 1) : 0;
    //int downPad=(srcHeight<destHeight)?(destHeight-srcHeight-upPad):0;
    int rows = (srcHeight > destHeight) ? destHeight : srcHeight;

    s = (char *)src;
    y = dest;
    u = y + destWidth * destHeight;
    v = u + ((destWidth * destHeight) >> 2);

    // handle the y up padding
    y += (destWidth * upPad);
    u += ((destWidth * upPad) >> 2);
    v += ((destWidth * upPad) >> 2);

    // handle the y up clipping
    s += ((srcWidth * upClip) << 1);

    if (leftPad != 0 || rightPad != 0) { // if padding non-zero
      // packed representation is UYUV UYUV UYUV
      for (a = (rows >> 1); a > 0; a--) {
	y += leftPad;
	u += (leftPad >> 1);
	v += (leftPad >> 1);
	// The information we have is 4:2:2. The subsampling consists in
	// keeping the chroma info from one line and throwing it out from
	// the next one. This is indeed 4:2:0 subsampling
	for (b = (srcWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(y++) = *(s++);
	  *(v++) = *(s++);
	  *(y++) = *(s++);
	  *(u++) = *(s++);
#else
	  *(u++) = *(s++);
	  *(y++) = *(s++);
	  *(v++) = *(s++);
	  *(y++) = *(s++);
#endif
	}
	y += rightPad;
	u += (rightPad >> 1);
	v += (rightPad >> 1);
	y += leftPad;
	for (b = (srcWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(y++) = *(s++);
	  s++;
	  *(y++) = *(s++);
	  s++;
#else
	  s++;
	  *(y++) = *(s++);
	  s++;
	  *(y++) = *(s++);
#endif
	}
	y += rightPad;
      }
    } else { // clipping is non-zero
      // packed representation is UYUV UYUV UYUV
      for (a = (rows >> 1); a > 0; a--) {
	s += (leftClip << 1);
	// The information we have is 4:2:2. The subsampling consists in
	// keeping the chroma info from one line and throwing it out from
	// the next one. This is indeed 4:2:0 subsampling
	for (b = (destWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(y++) = *(s++);
	  *(v++) = *(s++);
	  *(y++) = *(s++);
	  *(u++) = *(s++);
#else
	  *(u++) = *(s++);
	  *(y++) = *(s++);
	  *(v++) = *(s++);
	  *(y++) = *(s++);
#endif
	}
	s += (rightClip << 1);
	s += (leftClip << 1);
	for (b = (destWidth >> 1); b > 0; b--) {
#if BYTE_ORDER == BIG_ENDIAN
	  *(y++) = *(s++);
	  s++;
	  *(y++) = *(s++);
	  s++;
#else
	  s++;
	  *(y++) = *(s++);
	  s++;
	  *(y++) = *(s++);
#endif
	}
	s += (rightClip << 1);
      }
    }
  }
  return true;
}
