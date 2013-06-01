/*
 * Copyright (c) 1996 The Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
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
 *
 * This module contributed by John Brezak <brezak@apollo.hp.com>.
 * January 31, 1996
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <tcl.h>

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0)

#include "tkWinInt.h"

XImage *
XGetSubImage(
    Display*		display,
    Drawable		d,
    int			x,
    int			y,
    unsigned int	width,
    unsigned int	height,
    unsigned long	plane_mask,
    int			format,
    XImage*		dest_image,
    int			dest_x,
    int			dest_y
)
{
    printf("XGetSubImage: not implemented!\n");
    return(NULL);
}

unsigned long *win32Colors;
int win32NColors = 0;

void
XPutImage(
    Display*		display,
    Drawable		d,
    GC			gc,
    XImage*		image,
    int			src_x,
    int			src_y,
    int			dest_x,
    int			dest_y,
    unsigned int	width,
    unsigned int	height
)
{
    TkPutImage(win32Colors, win32NColors,
	       display, d, gc, image, src_x, src_y, dest_x, dest_y,
	       width, height);
}

/*
 * These functions belong in the TkWin port code
 */

/*
 * Translation table between X gc functions and Win32 raster op modes.
 */

static int ropModes[] = {
    R2_BLACK,			/* GXclear */
    R2_MASKPEN,			/* GXand */
    R2_MASKPENNOT,		/* GXandReverse */
    R2_COPYPEN,			/* GXcopy */
    R2_MASKNOTPEN,		/* GXandInverted */
    R2_NOT,			/* GXnoop */
    R2_XORPEN,			/* GXxor */
    R2_MERGEPEN,		/* GXor */
    R2_NOTMERGEPEN,		/* GXnor */
    R2_NOTXORPEN,		/* GXequiv */
    R2_NOT,			/* GXinvert */
    R2_MERGEPENNOT,		/* GXorReverse */
    R2_NOTCOPYPEN,		/* GXcopyInverted */
    R2_MERGENOTPEN,		/* GXorInverted */
    R2_NOTMASKPEN,		/* GXnand */
    R2_WHITE			/* GXset */
};

/*
 *----------------------------------------------------------------------
 *
 * XClearArea --
 *
 *	Clears the specified area in the window to the current
 * 	background color.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Erases the specified area of the window.
 *
 *----------------------------------------------------------------------
 */
void
XClearArea(display, w, x ,y, width, height, exposures)
    Display* display;
    Window w;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    Bool exposures;
{
    RECT rc;
    HBRUSH brush;
    HPALETTE oldPalette, palette;
    TkWindow *winPtr;
    HWND hwnd = TkWinGetHWND(w);
    HDC dc = GetDC(hwnd);

    palette = TkWinGetPalette(display->screens[0].cmap);
    oldPalette = SelectPalette(dc, palette, FALSE);

    display->request++;

    winPtr = TkWinGetWinPtr(w);
    brush = CreateSolidBrush(winPtr->atts.background_pixel);
    GetWindowRect(hwnd, &rc);
    rc.right = rc.right - rc.left;
    rc.bottom = rc.bottom - rc.top;
    rc.left = rc.top = 0;

    if (width!=0) {
	rc.left=(LONG)x;
	rc.right=(LONG)(x + width + 1);
    }

    if (height !=0) {
	rc.top=(LONG)y;
	rc.bottom=(LONG)(y + height + 1);
    }

    FillRect(dc, &rc, brush);

    DeleteObject(brush);
    SelectPalette(dc, oldPalette, TRUE);
    ReleaseDC(hwnd, dc);
}

/*
 *----------------------------------------------------------------------
 *
 * XDrawSegments --
 *
 *	Draw line segments
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Renders a series of line segments.
 * 
 * WARNING - stippled fill styles are not implemented.
 *
 *----------------------------------------------------------------------
 */
/*
void
XDrawSegments(display, d, gc, segments, nsegments)
    Display* display;
    Drawable d;
    GC gc;
    XSegment* segments;
    int nsegments;
{
    HPEN pen;
    TkWinDCState state;
    HDC dc;
    HPEN oldPen;
    HBRUSH oldBrush;
    int n;
    
    if (d == None) {
	return;
    }

    if ((gc->fill_style == FillStippled
	    || gc->fill_style == FillOpaqueStippled)
	    && gc->stipple != None) {

	panic("XDrawSegments: unimplemented stippled style");
    }

    dc = TkWinGetDrawableDC(display, d, &state);
    pen = CreatePen(PS_SOLID, gc->line_width, gc->foreground);

    oldPen = SelectObject(dc, pen);
    oldBrush = SelectObject(dc, CreateSolidBrush(gc->foreground));
    SetROP2(dc, ropModes[gc->function]);

    SetPolyFillMode(dc, (gc->fill_rule == EvenOddRule) ? ALTERNATE
		    : WINDING);

    for (n = 0; n < nsegments; n++) {
	MoveToEx(dc, (segments+n)->x1, (segments+n)->y1, NULL);
	LineTo(dc, (segments+n)->x2, (segments+n)->y2);
    }

    SelectObject(dc, oldPen);

    DeleteObject(SelectObject(dc, oldBrush));
    DeleteObject(pen);
    
    TkWinReleaseDrawableDC(d, dc, &state);
}
*/
#endif
