#
# Copyright (c) 1993-1995 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

#
# This file contains stubs for building any grabber specific controls.
# If a device is named foo, then create define build.foo proc here
# that builds UI.  If this proc doesn't exist, then it will be ommitted.
# If there are multiple devices with the same name, they should be
# called foo-1, foo-2, etc. and you'll only need build.foo
#

proc build.slicvideo w {
	set f [smallfont]
	label $w.title -text "Grabber"
	frame $w.f -relief sunken -borderwidth 2

	frame $w.f.h -relief flat
       
	label $w.f.h.label  -font $f -anchor e -text "Hue"
       
	scale $w.f.h.scale -orient horizontal -width 12 -length 20 \
		           -relief groove -showvalue 0 -from -128 -to 127 \
                          -command "grabber set HUE"
	pack  $w.f.h.label $w.f.h.scale -side left -fill x -expand 1

	frame $w.f.ll -relief flat 
       
	label $w.f.ll.label  -font $f -text "Luma" -anchor s
       
	label $w.f.ll.clabel -font $f -text "Contrast" -anchor s
      
	label $w.f.ll.blabel -font $f -text "Brightness" -anchor s
	pack  $w.f.ll.clabel $w.f.ll.label $w.f.ll.blabel \
			     -side left -fill x -expand 1

	frame $w.f.l  -relief flat
       
	scale $w.f.l.cscale   -orient horizontal -width 12 -relief groove \
                              -showvalue 0 -from 0 -to 127 \
                              -command "grabber set LUMA_CONTRAST"
       
	scale $w.f.l.bscale -orient horizontal -width 12 -relief groove \
                            -showvalue 0 -from 0 -to 255 \
                            -command "grabber set LUMA_BRIGHTNESS"
	pack  $w.f.l.cscale $w.f.l.bscale  -side left -fill x -expand 1

	frame $w.f.cl  -relief flat

	label $w.f.cl.label  -font $f -text "Chroma" -anchor n
       
	label $w.f.cl.glabel -font $f -text "Gain" -anchor n
       
	label $w.f.cl.slabel -font $f -text "Saturation" -anchor n
	pack  $w.f.cl.glabel $w.f.cl.label $w.f.cl.slabel \
			     -side left -fill x -expand 1

	frame $w.f.c -relief flat
       
	scale $w.f.c.gscale -orient horizontal -width 12 -relief groove \
                             -showvalue 0 -from 0 -to 255 \
                             -command "grabber set CHROMA_GAIN"
       
	scale $w.f.c.sscale -orient horizontal -width 12 -relief groove \
                            -showvalue 0 -from 0 -to 127 \
                            -command "grabber set CHROMA_SATURATION"
	pack  $w.f.c.gscale $w.f.c.sscale -side left -fill x -expand 1

     
	pack  $w.f.h $w.f.ll $w.f.l $w.f.c $w.f.cl \
	      -fill x -expand 1 -padx 1m 


	pack $w.title $w.f -fill x -expand 1

	$w.f.h.scale  set 0
	$w.f.l.cscale set 64
	$w.f.l.bscale set 128
	$w.f.c.gscale set 44
	$w.f.c.sscale set 64
}

proc build.still w {

    set f [smallfont]
    label $w.title -text "Grabber"

    frame $w.f -relief sunken -borderwidth 2
    
    label $w.f.label  -font $f -anchor e -text "File"
    
    mk.entry $w.f set.still.frame "frame"

    pack $w.title $w.f -fill x -expand 1
    
    pack $w.f.label -side left 
    pack $w.f.entry -side left -fill x -expand 1
}

proc set.still.frame {w s } {
    global lastDevice
    $lastDevice file $s
}

proc build.qcam {w} {
    global qcamwindow

    set f [smallfont]
    label $w.title -text "Grabber"

    frame $w.f -relief sunken -borderwidth 2

    frame $w.f.s -relief flat

    frame $w.f.s.l -relief flat
    label $w.f.s.l.bright -font $f -anchor w -text "Brightness"
    label $w.f.s.l.cont   -font $f -anchor w -text "Contrast"
    label $w.f.s.l.wbal   -font $f -anchor w -text "White balance"
    pack  $w.f.s.l.bright $w.f.s.l.cont $w.f.s.l.wbal \
	-side top -fill x -expand 1

    frame $w.f.s.s -relief flat
    scale $w.f.s.s.bright -orient horizontal -width 12 \
		          -relief groove -showvalue 0 -from 1 -to 254 \
                          -command "grabber set BRIGHT"
    scale $w.f.s.s.cont   -orient horizontal -width 12 \
                          -relief groove -showvalue 0 \
                          -from 0 -to 1.0 -resolution 0.002 \
                          -command "grabber contrast"

    frame $w.f.s.s.wbal -relief flat
    scale $w.f.s.s.wbal.scale  -orient horizontal -width 12 \
                             -relief groove -showvalue 0 -from 1 -to 254 \
                             -command "grabber set WBAL"
    button $w.f.s.s.wbal.button -font $f -text Auto \
	-command "grabber set WBAL auto"
    pack  $w.f.s.s.wbal.scale $w.f.s.s.wbal.button \
	-side left -fill x -expand 1

    pack $w.f.s.s.bright $w.f.s.s.cont $w.f.s.s.wbal \
        -side top -fill x -expand 1

    pack $w.f.s.l $w.f.s.s -side left -fill x -expand 1

    frame $w.f.bpp -relief flat
    label $w.f.bpp.label  -font $f -anchor w -text "Pixel depth"
    radiobutton $w.f.bpp.bpp4 -font $f -text "4-bit" \
	-variable qcambpp -value 4 -command "grabber set BPP 4"
    radiobutton $w.f.bpp.bpp6 -font $f -text "6-bit" \
	-variable qcambpp -value 6 -command "grabber set BPP 6"
    pack $w.f.bpp.label $w.f.bpp.bpp4 $w.f.bpp.bpp6 \
	-side left -fill x -expand 1

    pack  $w.f.s $w.f.bpp \
	 -fill x -expand 1 -padx 1m 

    pack $w.title $w.f -fill x -expand 1

    set qcamwindow(setbright) "$w.f.s.s.bright set"
    set qcamwindow(setcont) "$w.f.s.s.cont set"
    set qcamwindow(setwbal) "$w.f.s.s.wbal.scale set"
    set qcamwindow(setbpp) "set qcambpp"
}
