#
# Copyright (c) 1993-1995 The Regents of the University of California.
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
# 3. Neither the names of the copyright holders nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

proc build.dc10 w {
    build.v4l $w
}

proc build.v4l w {
    set f [smallfont]
    global contrast brightness hue saturation norm
    set contrast [resource contrast]
    set brightness [resource brightness]
    set hue [resource hue]
    set saturation [resource saturation]
    set norm 0

    label $w.title -text "Video4Linux grabber controls"
    pack $w.title  -fill x -expand 1

    frame $w.f -relief sunken -borderwidth 2
    frame $w.f.left -relief flat


    button $w.f.left.reset -font $f -width 10 -text "Reset" -command "set contrast 128; set brightness 128; set hue 128; set saturation 128; grabber controls reset"  -padx 1 -pady 1
    pack $w.f.left.reset

    frame $w.f.right -relief flat
    frame $w.f.right.top -relief flat

    frame $w.f.right.top.c -relief flat -borderwidth 2
    scale $w.f.right.top.c.cscale -orient horizontal -font $f -relief groove  -width 12  \
                -showvalue 0 -from 0 -to 255 \
                -variable contrast \
                -label Contrast \
                -command "grabber contrast"
    pack  $w.f.right.top.c.cscale  -fill x -expand 1

    frame $w.f.right.top.b -relief flat -borderwidth 2
    scale $w.f.right.top.b.bscale -orient horizontal -font $f -relief groove -width 12  \
                -showvalue 0 -from 0 -to 255 \
                -variable brightness \
                -label Brightness \
                -command "grabber brightness"
    pack  $w.f.right.top.b.bscale -fill x -expand 1
    frame $w.f.right.bottom -relief flat


    frame $w.f.right.bottom.h -relief flat -borderwidth 2
    scale $w.f.right.bottom.h.hscale -orient horizontal -font $f -relief groove  -width 12 \
                -showvalue 0 -from 0 -to 255 \
                -variable hue \
                -label Hue \
                -command "grabber hue"
    pack  $w.f.right.bottom.h.hscale -fill x -expand 1

    frame $w.f.right.bottom.s -relief flat -borderwidth  2
    scale $w.f.right.bottom.s.sscale -orient horizontal -font $f -relief groove  -width 12 \
                -showvalue 0 -from 0 -to 255 \
                -variable saturation \
                -label Saturation \
                -command "grabber saturation"
    pack  $w.f.right.bottom.s.sscale -fill x -expand 1

    pack $w.f.right.top.b $w.f.right.top.c -side right
    pack $w.f.right.bottom.s $w.f.right.bottom.h -side right
    pack $w.f.right.top $w.f.right.bottom -side top -fill x -expand 1
    pack $w.f.left $w.f.right -side left -expand 1 -fill x
    pack $w.f -expand 1 -fill x

}


proc build.v4l2 w {
    set f [smallfont]
    global contrast brightness v4l2gamma hue saturation v4l2gain norm
    global antiflicker
    set contrast [resource contrast]
    set brightness [resource brightness]
    set v4l2gamma [resource v4l2gamma]
    set hue [resource hue]
    set saturation [resource saturation]
    set v4l2gain [resource v4l2gain]
    set norm 0
    set antiflicker [resource antiflicker]

    label $w.title -text "Video4Linux2 grabber controls"
    pack $w.title  -fill x -expand 1

    frame $w.f -relief sunken -borderwidth 2
    frame $w.f.left -relief flat

    set m $w.f.left.antiflicker.menu
    menubutton $w.f.left.antiflicker -menu $m -text "Anti-flicker" \
	-relief raised -width 14 -font $f -padx 1 -pady 1 -indicatoron 1
    menu $m
    $m add radiobutton -label "disabled" -command "grabber antiflicker 0" \
	-value "disabled" -variable antiflicker -font $f
    $m add radiobutton -label "50 Hz" -command "grabber antiflicker 1" \
	-value "50Hz" -variable antiflicker -font $f
    $m add radiobutton -label "60 Hz" -command "grabber antiflicker 2" \
	-value "60Hz" -variable antiflicker -font $f
    pack $w.f.left.antiflicker

    button $w.f.left.reset -font $f -width 14 -text "Reset" -command "set contrast 128; set brightness 128;set v4l2gamma 128; set hue 128; set saturation 128; set v4l2gain 128; grabber controls reset"  -padx 1 -pady 1
    pack $w.f.left.reset

    frame $w.f.right -relief flat
    frame $w.f.right.top -relief flat

    frame $w.f.right.top.contrast -relief flat -borderwidth 2
    scale $w.f.right.top.contrast.scale -orient horizontal -font $f -relief groove  -width 12  \
                -showvalue 0 -from 0 -to 255 \
                -variable contrast \
                -label Contrast \
                -command "grabber contrast"
    pack  $w.f.right.top.contrast.scale  -fill x -expand 1

    frame $w.f.right.top.brightness -relief flat -borderwidth 2
    scale $w.f.right.top.brightness.scale -orient horizontal -font $f -relief groove -width 12  \
                -showvalue 0 -from 0 -to 255 \
                -variable brightness \
                -label Brightness \
                -command "grabber brightness"
    pack  $w.f.right.top.brightness.scale -fill x -expand 1

    frame $w.f.right.top.gamma -relief flat -borderwidth 2
    scale $w.f.right.top.gamma.scale -orient horizontal -font $f -relief groove  -width 12 \
                -showvalue 0 -from 0 -to 255 \
                -variable v4l2gamma \
                -label Gamma \
                -command "grabber gamma"
    pack  $w.f.right.top.gamma.scale -fill x -expand 1
    frame $w.f.right.bottom -relief flat

    frame $w.f.right.bottom.hue -relief flat -borderwidth 2
    scale $w.f.right.bottom.hue.scale -orient horizontal -font $f -relief groove  -width 12 \
                -showvalue 0 -from 0 -to 255 \
                -variable hue \
                -label Hue \
                -command "grabber hue"
    pack  $w.f.right.bottom.hue.scale -fill x -expand 1

    frame $w.f.right.bottom.saturation -relief flat -borderwidth  2
    scale $w.f.right.bottom.saturation.scale -orient horizontal -font $f -relief groove  -width 12 \
                -showvalue 0 -from 0 -to 255 \
                -variable saturation \
                -label Saturation \
                -command "grabber saturation"
    pack  $w.f.right.bottom.saturation.scale -fill x -expand 1

    frame $w.f.right.bottom.gain -relief flat -borderwidth  2
    scale $w.f.right.bottom.gain.scale -orient horizontal -font $f -relief groove  -width 12 \
                -showvalue 0 -from 0 -to 255 \
                -variable v4l2gain \
                -label Gain \
                -command "grabber gain"
    pack  $w.f.right.bottom.gain.scale -fill x -expand 1

    pack $w.f.right.top.brightness $w.f.right.top.contrast $w.f.right.top.gamma -side right
    pack $w.f.right.bottom.saturation $w.f.right.bottom.hue $w.f.right.bottom.gain -side right
    pack $w.f.right.top $w.f.right.bottom -side top -fill x -expand 1
    pack $w.f.left $w.f.right -side left -expand 1 -fill x
    pack $w.f -expand 1 -fill x
}

proc build.meteor w {
#
# The meteor has the same controls as the slicvideo, so just call that
# routine to build the controls.

	build.slicvideo $w
}

proc build.bktr w {
	build.slicvideo $w
}

proc build.parallax w {
global logoButton, setPlxInput, V
	set f [smallfont]
	label $w.title -text "Parallax-Grabber"
	frame $w.f -relief sunken -borderwidth 2

	frame $w.f.hl -relief flat

	label $w.f.hl.label  -font $f -text "Hue" -anchor s 

	label $w.f.hl.slabel -font $f -text "Saturation" -anchor s

	pack  $w.f.hl.label $w.f.hl.slabel -side left -fill x -expand 1

	frame $w.f.h -relief flat

	scale $w.f.h.hscale -orient horizontal -width 12 \
		-relief groove -showvalue 1 -from 0 -to 255 \
		-variable parallaxHue \
		-command "grabber hue"
	scale $w.f.h.sscale -orient horizontal -width 12 -relief groove \
		-showvalue 1 -from 0 -to 255 \
		-variable parallaxSat \
		-command "grabber saturation"
	pack  $w.f.h.hscale $w.f.h.sscale -side left -fill x -expand 1

	frame $w.f.ll -relief flat

	label $w.f.ll.clabel -font $f -text "Contrast" -anchor s

	label $w.f.ll.blabel -font $f -text "Brightness" -anchor s
	pack  $w.f.ll.clabel $w.f.ll.blabel -side left -fill x -expand 1

	frame $w.f.l  -relief flat

	scale $w.f.l.cscale   -orient horizontal -width 12 -relief groove \
		-showvalue 1 -from 0 -to 255 \
		-variable parallaxCon \
		-command "grabber contrast"

	scale $w.f.l.bscale -orient horizontal -width 12 -relief groove \
		-showvalue 1 -from 0 -to 255 \
		-variable parallaxBri \
		-command "grabber brightness"
	pack  $w.f.l.cscale $w.f.l.bscale  -side left -fill x -expand 1

	frame $w.f.qb  -relief flat

	set logoButton $w.f.qb.logo
	button $w.f.qb.logo -text "Logo" \
		-relief raised -command "grabber set_logo"\
		-anchor w -font $f \
		-width 5 -state normal -highlightthickness 0

	button $w.f.qb.halfbutton -font $f -text Halfsize -width 9 -command "grabber half"

	radiobutton $w.f.qb.b0 -text "PAL" -command "grabber setInput PAL" \
		-padx 0 -pady 0 \
		-anchor w -variable setPlxInput -font $f -relief flat -value "PAL"
	radiobutton $w.f.qb.b1 -text "NTSC" -command "grabber setInput NTSC" \
		-padx 0 -pady 0 \
		-anchor w -variable setPlxInput -font $f -relief flat -value "NTSC"
	radiobutton $w.f.qb.b2 -text "SECAM" -command "grabber setInput SECAM" \
		-padx 0 -pady 0 \
		-anchor w -variable setPlxInput -font $f -relief flat -value "SECAM"
	pack $w.f.qb.b0 $w.f.qb.b1 $w.f.qb.b2 -fill x -side left


	pack  $w.f.qb.logo $w.f.qb.halfbutton -side left -fill x 

	frame $w.f.rb  -relief flat

	label $w.f.rb.label -text "Remote-Display:" -font $f -anchor w -padx 1m
# XXX
	mk.entry $w.f.rb grabber [option get . localPlxDisplay $V(class)]
	$w.f.rb.entry configure -state disabled -foreground gray60
	$w.f.rb.label configure -foreground gray60

	pack  $w.f.rb.label $w.f.rb.entry -side left -fill x -expand 1
	pack  $w.f.hl $w.f.h $w.f.ll $w.f.l $w.f.qb $w.f.rb -fill x -padx 1m


	frame $w.f.lr  -relief flat
	frame $w.f.lr.x  -relief flat
	frame $w.f.lr.y  -relief flat

	label $w.f.lr.x.label -text "Capture-Window-Size: " -font $f -anchor w -padx 1m
	mk.entry $w.f.lr.x grabber  ""
	$w.f.lr.x.entry configure -width 6

	label $w.f.lr.y.label -text "x" -font $f -anchor e 
	mk.entry $w.f.lr.y grabber ""
	$w.f.lr.y.entry configure -width 6

	checkbutton $w.f.lr.stillbutton -text "Not Live" \
		-relief raised -command "grabber sendStill" \
		-anchor e -variable stillButtonState -font $f \
		-state normal -highlightthickness 0

	pack  $w.f.lr.x.label -side left -padx 1m
	pack  $w.f.lr.x.entry -side left 
	pack  $w.f.lr.y.label -side left 
	pack  $w.f.lr.y.entry -side left 
	pack  $w.f.lr.x $w.f.lr.y $w.f.lr -side left 
	pack  $w.f.lr.stillbutton -side left 


	pack  $w.title $w.f -fill x -expand 1

	$w.f.h.hscale set 128
	$w.f.h.sscale set 128
	$w.f.l.cscale set 128
	$w.f.l.bscale set 128
	set logoButtonState 0
	set stillButtonState 0
	set setPlxInput "PAL"
}


proc build.xil_SlicVideo w {
	# .menu.xil
	set f [smallfont]
	label $w.title -text "XIL SLIC Grabber"
	frame $w.f -relief sunken -borderwidth 2

	frame $w.f.left
	frame $w.f.right

	frame $w.f.left.hue -relief flat
	label $w.f.left.hue.name -font $f -anchor e -text "Hue"
	scale $w.f.left.hue.scale -orient horizontal -width 12 \
			   -variable xilHue \
		           -relief groove -showvalue 1 -from -128 -to 127 \
                           -command "grabber set HUE"
	pack $w.f.left.hue.name $w.f.left.hue.scale -fill x -expand 1

	frame $w.f.left.lbrightness -relief flat
	label $w.f.left.lbrightness.name -font $f -anchor e -text "Luma Brightness"
	scale $w.f.left.lbrightness.scale -orient horizontal -width 12 \
			   -variable xilLumaBrightness \
		           -relief groove -showvalue 1 -from 0 -to 255 \
                           -command "grabber set LUMA_BRIGHTNESS"
	pack $w.f.left.hue $w.f.left.lbrightness.name $w.f.left.lbrightness.scale -fill x -expand 1


	frame $w.f.left.lcontrast -relief flat
	label $w.f.left.lcontrast.name -font $f -anchor e -text "Luma Contrast"
	scale $w.f.left.lcontrast.scale -orient horizontal -width 12 \
		           -relief groove -showvalue 1 -from 0 -to 127 \
			   -variable xilLumaContrast \
			   -command "grabber set LUMA_CONTRAST"
	pack $w.f.left.lcontrast.name $w.f.left.lcontrast.scale -fill x -expand 1

	pack $w.f.left.lbrightness $w.f.left.lcontrast -side left -fill x -expand 1 


	frame $w.f.right.csat -relief flat
	label $w.f.right.csat.name -font $f -anchor e -text "Chroma Saturation"
	scale $w.f.right.csat.scale -orient horizontal -width 12 \
		           -relief groove -showvalue 1 -from 0 -to 127 \
			   -variable xilChromaSaturation \
                           -command "grabber set CHROMA_SATURATION"
	pack $w.f.right.csat.name $w.f.right.csat.scale -fill x -expand 1

	frame $w.f.right.cgain -relief flat
	label $w.f.right.cgain.name -font $f -anchor e -text "Chroma Gain"
	scale $w.f.right.cgain.scale -orient horizontal -width 12 \
		           -relief groove -showvalue 1 -from 0 -to 255 \
			   -variable xilChromaGain \
                          -command "grabber set CHROMA_GAIN"
	pack $w.f.right.cgain.name $w.f.right.cgain.scale -fill x -expand 1


	pack $w.f.right.csat $w.f.right.cgain -side left -fill x -expand 1 

	pack $w.f.left $w.f.right -fill y -side top -expand 1

	pack $w.title $w.f -fill x -expand 1

	set xcsat [resource chroma_saturation]
	if { $xcsat != 0 } {
		$w.f.right.csat.scale set $xcsat
	} else {
		$w.f.right.csat.scale set 64
	}
	set xcgain [resource chroma_gain]
	if { $xcgain != 0} {
		$w.f.right.cgain.scale  set $xcgain
	} else {
		$w.f.right.csat.scale set 44
	}
	set xlcontrast [resource luma_contrast]
	if { $xlcontrast != 0 }  {
		$w.f.left.lcontrast.scale  set $xlcontrast
	} else {
		$w.f.left.lcontrast.scale  set 64
	}
	set xlbrightness [resource luma_brightness]
	if { $xlbrightness }  {
		$w.f.left.lbrightness.scale  set $xlbrightness
	} else {
		$w.f.left.lbrightness.scale  set 128
	}
}

proc xil_set_vformat { value } {
	global env

	if [info exists env(O1KHOME)] {
        	if { [file exists "$env(O1KHOME)/bin/o1k_ctl"] } {
                 exec $env(O1KHOME)/bin/o1k_ctl -D /dev/o1k0 -cmd -set -vf $value
                 restart
		 return
		}
	} 
	if { [file exists "/opt/MMACo1k/bin/o1k_ctl"] } {
		exec /opt/MMACo1k/bin/o1k_ctl -D /dev/o1k0 -cmd -set -vf $value
		restart
	} elseif { [file exists "/opt/SUNWo1kp/bin/o1k_ctl"] } {
		exec /opt/SUNWo1kp/bin/o1k_ctl -D /dev/o1k0 -cmd -set -vf $value
		restart
	} else {
		puts "Cannot find config utility"
	}
}

proc xil_set_hsbc { w param value } {
	$w configure -text $value
	grabber set $param $value
}

proc build.xil_Osprey_1k w {
	# .menu.xil
	global xilHue xilSaturation xilBrightness xilContrast
	set f [smallfont]
	label $w.title -text "XIL Grabber"
	frame $w.f -relief sunken -borderwidth 2

	frame $w.f.left
	frame $w.f.right
	frame $w.f.names
	frame $w.f.vformat -relief groove -borderwidth 2 

	frame $w.f.hue -relief flat
	label $w.f.names.hue -justify left -font $f -anchor w -text "Hue"
	label $w.f.hue.val -font $f -text 0 -width 4
	scale $w.f.hue.scale -orient horizontal -width 12 \
			   -variable xilHue \
		           -relief groove -showvalue 0 -from -128 -to 127 \
                           -command "xil_set_hsbc $w.f.hue.val HUE"
#                           -command " $w.f.hue.val configure -text \$xilHue
#        grabber set HUE "
#                           -command "xil_grabber_set HUE $w.f.hue.val"
	pack $w.f.hue.scale $w.f.hue.val -fill x -expand 1 -side left

#	frame $w.f.left.chromau -relief flat
#	label $w.f.left.chromau.name -font $f -anchor e -text "Gain U"
#	scale $w.f.left.chromau.scale -orient horizontal -width 12 \
#		           -relief groove -showvalue 1 -from -256 -to 255 \
#			   -variable xilChromaGainU \
#                          -command "grabber set CHROMA_GAIN_U"
#	pack $w.f.left.chromau.name $w.f.left.chromau.scale -fill x -expand 1
#
#	frame $w.f.left.chromav -relief flat
#	label $w.f.left.chromav.name -font $f -anchor e -text "Gain V"
#	scale $w.f.left.chromav.scale -orient horizontal -width 12 \
#		           -relief groove -showvalue 1 -from -256 -to 255 \
#			   -variable xilChromaGainV \
#                           -command "grabber set CHROMA_GAIN_V"
#	pack $w.f.left.chromav.name $w.f.left.chromav.scale -fill x -expand 1

	frame $w.f.saturation -relief flat
	label $w.f.names.saturation -justify left -font $f -anchor w -text "Saturation"
	label $w.f.saturation.val -font $f -text 0 -width 4
	scale $w.f.saturation.scale -orient horizontal -width 12 \
		           -relief groove -showvalue 0 -from -256 -to 255 \
			   -variable xilSaturation \
                           -command "xil_set_hsbc $w.f.saturation.val SATURATION"
#                          -command "$w.f.saturation.val configure -text \$xilSaturation 
#grabber set SATURATION "
	pack $w.f.saturation.scale $w.f.saturation.val -fill x -expand 1 -side left


	#pack $w.f.hue $w.f.saturation -side top -fill x -expand 1 -anchor w

	frame $w.f.brightness -relief flat
	label $w.f.names.brightness -justify left -font $f -anchor w -text "Brightness"
	label $w.f.brightness.val  -font $f -text 0 -width 4
	scale $w.f.brightness.scale -orient horizontal -width 12 \
		           -relief groove -showvalue 0 -from -128 -to 127 \
			   -variable xilBrightness \
                           -command "xil_set_hsbc $w.f.brightness.val BRIGHTNESS"
	pack $w.f.brightness.scale $w.f.brightness.val -fill x -expand 1 -side left

	frame $w.f.contrast -relief flat
	label $w.f.names.contrast -justify left -font $f -anchor w -text "Contrast"
	label $w.f.contrast.val -font $f -text 0 -width 4
	scale $w.f.contrast.scale -orient horizontal -width 12 \
			   -variable xilContrast \
		           -relief groove -showvalue 0 -from -256 -to 255 \
                           -command "xil_set_hsbc $w.f.contrast.val CONTRAST"
	pack  $w.f.contrast.scale $w.f.contrast.val -fill x -expand 1 -side left

	#pack $w.f.brightness $w.f.contrast -side top -fill x -expand 1 -anchor w
	label $w.f.vformat.title -text "Video Format" -font $f

	radiobutton $w.f.vformat.pal -text Pal -command "xil_set_vformat 0" -anchor w -padx 4 -font $f -variable vformat -value 1
	radiobutton $w.f.vformat.ntsc -text Ntsc -command "xil_set_vformat 1" -anchor w -padx 4 -font $f -variable vformat -value 2
	radiobutton $w.f.vformat.auto -text Auto -command "xil_set_vformat 2" -anchor w -padx 4 -font $f -variable vformat -value 0

	pack $w.f.left -fill y -side left -expand 1
	pack $w.f.vformat.title $w.f.vformat.pal $w.f.vformat.ntsc -side top -anchor w
	pack $w.f.vformat -fill y -side right -expand 1 -pady 4
	pack $w.f.right -fill y -side right -expand 1

	pack $w.f.names.hue $w.f.names.saturation $w.f.names.brightness $w.f.names.contrast -side top -anchor w
	pack $w.f.names -in $w.f.left -side top
	pack $w.f.hue $w.f.saturation $w.f.brightness $w.f.contrast -in $w.f.right -side top
	pack $w.title $w.f -fill x -expand 1

	set xhue [resource hue]
	if { $xhue != 0 } {
		$w.f.hue.scale set $xhue
	}
	#set xchromau [resource chromau]
	#if { $xchromau != 0} {
	#	$w.f.left.chromau.scale  set $xchromau
	#}
	set xsaturation [resource saturation]
	if { $xsaturation != 0 }  {
		$w.f.saturation.scale  set $xsaturation
	}
	#set xchromav [resource chromav]
	#if { $xchromav }  {
	#	$w.f.left.chromav.scale  set $xchromav
	#}
	set xbrightness [resource brightness]
	if { $xbrightness!=0 } {
		$w.f.brightness.scale  set $xbrightness
	}
	set xcontrast [resource contrast]
	if { $xcontrast != 0 } {
		$w.f.contrast.scale  set $xcontrast
	}
}


proc build.slicvideo w {
	set f [smallfont]
	label $w.title -text "Grabber"
	frame $w.f -relief sunken -borderwidth 2

        frame $w.f.h -relief flat
	frame $w.f.h.c
	label $w.f.h.c.l1  -font $f -anchor e -text "Chan:"
	mk.entry $w.f.h.c  do_chan "20"
	$w.f.h.c.entry configure -relief sunken -width 5
	bind $w.f.h.c.entry <Return> "grabber chan \[$w.f.h.c.entry get\]"
	pack $w.f.h.c.l1 $w.f.h.c.entry -side top

	global ths
	scale $w.f.h.ths \
-orient horizontal -width 12 -length 60 -relief groove -sliderlength 6 \
	    -showvalue 0 -from 1 -to 10 \
	    -command "grabber threshold" -label Thre
	$w.f.h.ths set 6
	# the actual scale is multiplied by 8
	scale $w.f.h.scale \
-orient horizontal -width 12 -length 100 -relief groove \
	    -showvalue 0 -from -128 -to 127 \
	    -command "grabber hue" -label Hue
	pack  $w.f.h.c \
		$w.f.h.ths \
		$w.f.h.scale \
			     -side left -fill x -expand 1
#	frame $w.f.ll -relief flat 
#	label $w.f.ll.label  -font $f -text "Luma" -anchor s
#	label $w.f.ll.clabel -font $f -text "Contrast" -anchor s
#	label $w.f.ll.blabel -font $f -text "Brightness" -anchor s
#	pack  $w.f.ll.clabel $w.f.ll.label $w.f.ll.blabel \
#			     -side left -fill x -expand 1

	frame $w.f.l  -relief flat
       
	scale $w.f.l.cscale \
-orient horizontal -width 12 -length 100 -relief groove \
                              -showvalue 0 -from 0 -to 127 \
	  -label "Contrast" \
	  -command "grabber contrast"
       
	scale $w.f.l.bscale \
-orient horizontal -width 12 -length 100 -relief groove \
                            -showvalue 0 -from 0 -to 255 \
	    -command "grabber brightness" -label "Brightness"
	pack  $w.f.l.cscale $w.f.l.bscale  -side left -fill x -expand 1

#	frame $w.f.cl  -relief flat
#	label $w.f.cl.label  -font $f -text "Chroma" -anchor n
#	label $w.f.cl.glabel -font $f -text "Gain" -anchor n
#	label $w.f.cl.slabel -font $f -text "Saturation" -anchor n
#	pack  $w.f.cl.glabel $w.f.cl.label $w.f.cl.slabel \
#			     -side left -fill x -expand 1

	frame $w.f.c -relief flat
       
	scale $w.f.c.gscale \
-orient horizontal -width 12 -length 100 -relief groove \
                             -showvalue 0 -from 0 -to 255 \
	     -command "grabber uvgain" -label "Chr.  Gain"
       
	scale $w.f.c.sscale \
-orient horizontal -width 12 -length 100 -relief groove \
                            -showvalue 0 -from 0 -to 127 \
	    -command "grabber saturation" -label "Saturation"
	pack  $w.f.c.gscale $w.f.c.sscale -side left -fill x -expand 1

     
	# pack  $w.f.h $w.f.ll $w.f.l $w.f.c $w.f.cl
	pack  $w.f.h $w.f.l $w.f.c -fill x -expand 1 -padx 1m 


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

proc build.filedev w {
	set f [smallfont]
	label $w.title -text "FileGrabber"

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

proc build.ov511 {w} {
    global ov511window

    set f [smallfont]
    label $w.title -text "Grabber"

    frame $w.f -relief sunken -borderwidth 2

    frame $w.f.s -relief flat

    frame $w.f.s.l -relief flat
    label $w.f.s.l.red   -font $f -anchor w -text "Red balance"
    label $w.f.s.l.blue   -font $f -anchor w -text "Blue balance"
    pack $w.f.s.l.red $w.f.s.l.blue -side top -fill x -expand 1

    frame $w.f.s.s -relief flat
    scale $w.f.s.s.red    -orient horizontal -width 12 \
		          -relief groove -showvalue 0 -from 0 -to 255 \
                          -command "grabber set RED"
    scale $w.f.s.s.blue   -orient horizontal -width 12 \
		          -relief groove -showvalue 0 -from 0 -to 255 \
                          -command "grabber set BLUE"

    pack $w.f.s.s.red $w.f.s.s.blue -side top -fill x -expand 1
    pack $w.f.s.l $w.f.s.s -side left -fill x -expand 1

    pack $w.f.s -fill x -expand 1
    pack $w.title $w.f -fill x -expand 1

    set ov511window(setred) "$w.f.s.s.red set"
    set ov511window(setblue) "$w.f.s.s.blue set"
}

#
# X11 Grabber controls
#
proc x11grabUpdatePos {x y w h} {

    global x11grabcontrols
    set w $x11grabcontrols

    if {[string compare $x [$w.x11grab.row1.pos.x.e get]] != 0} {
	$w.x11grab.row1.pos.x.e delete 0 end
	$w.x11grab.row1.pos.x.e insert 0 $x
    }
    if {[string compare $y [$w.x11grab.row1.pos.y.e get]] != 0} {
	$w.x11grab.row1.pos.y.e delete 0 end
	$w.x11grab.row1.pos.y.e insert 0 $y
    }
    if {[string compare $w [$w.x11grab.row1.pos.w.e get]] != 0} {
	$w.x11grab.row1.pos.w.e delete 0 end
	$w.x11grab.row1.pos.w.e insert 0 $w
    }
    if {[string compare $h [$w.x11grab.row1.pos.h.e get]] != 0} {
	$w.x11grab.row1.pos.h.e delete 0 end
	$w.x11grab.row1.pos.h.e insert 0 $h
    }
}

proc x11cmd.update.geo w {
    grabber fixed [$w.row.x get]  [$w.row.y get]
}

proc x11cmd.fixed {} {
    global x11Source x11grabcontrols
    set w $x11grabcontrols
    $w.label configure -text "$x11Source"
    if [winfo exists $w.row] {
	destroy $w.row
    }
    frame $w.row
    pack append $w.row \
	[label $w.row.xl -text "X:" -width 2 -anchor e] {left filly} \
	[entry $w.row.x -relief flat -width 4] {left filly} \
	[label $w.row.yl -text "Y:" -width 2 -anchor e] {left filly} \
	[entry $w.row.y -relief flat -width 4] {left filly}
    bind $w.row.x <Return> "x11cmd.update.geo $w"
    bind $w.row.y <Return> "x11cmd.update.geo $w"

    pack $w.row -after $w.label
}

proc x11cmd.pointer {} {
    global x11Source x11grabcontrols
    set w $x11grabcontrols
    $w.label configure -text "$x11Source"
    if [winfo exists $w.row] {
	destroy $w.row
    }
    frame $w.row
    pack append $w.row \
	[button $w.row.s -text "Follow pointer" ] { left filly }
    pack $w.row -after $w.label
}

proc x11cmd.window {} {
	global x11Source x11grabcontrols
	puts "x11cmd -- x11Source $x11Source"
	set w $x11grabcontrols
	$w.label configure -text "$x11Source"
	if [winfo exists $w.row] {
	    destroy $w.row
	}
	frame $w.row
	pack append $w.row \
	    [button $w.row.s -text "Select window" ] { left filly }
	pack $w.row -after $w.label
}

proc build.x11 w {
	global x11grabcontrols x11Source
	set f [smallfont] 

	label $w.title -text "X11 Grabber controls"
	frame $w.x11grab -relief sunken -borderwidth 2
	set x11grabcontrols $w.x11grab
	set x11Source "Fixed"
	set w1 $w.x11grab

	
	# luigi
	set m $w1.menu
	set m1 $m.m1
	menubutton $w1.menu -menu $m1 -text "Source:" \
		-relief raised -width 7 -font $f -indicatoron 1
	label $w1.label -width 6 -font $f
	frame $w1.row
	menu $m1
	$m1 add radiobutton -label Fixed \
		-state active \
		-command "x11cmd.fixed" -font $f -variable x11Source
#	$m1 add radiobutton -label Pointer \
#		-command "x11cmd.pointer" -font $f -variable x11Source
#	$m1 add radiobutton -label Window \
#		-command "x11cmd.window" -font $f -variable x11Source

	pack append $w1 \
		$w1.menu {left} \
		$w1.label {left} \
		$w1.row {left}
	
	pack $w $w.title $w1 -fill x -expand 1

	x11cmd.fixed
}

proc build.cosmo w {
	global fieldButtonState
	set fieldButtonState 0

	set f [smallfont]
	label $w.title -text "Grabber"

	frame $w.f -relief sunken -borderwidth 2

	frame $w.f.b -relief flat
       
# would be ok, if we could change the button text after pressing, hm.
	button $w.f.b.fieldbutton -text "Toggle send both/even fields" \
		-relief raised -command "grabber even_only" \
		-anchor w -font $f \
		-width 20 \
		-state normal -highlightthickness 0

# would be nice if this would work, sigh
#	checkbutton $w.f.b.fieldbutton -text "Send both fields" \
#	-relief raised -command "grabber even_only $fieldButtonState" \
#	-anchor w -variable fieldButtonState -font $f \
#	-state normal -highlightthickness 0

	pack  $w.f.b $w.f.b.fieldbutton -fill x -padx 1m
	pack $w.title $w.f -fill x -expand 1
}
