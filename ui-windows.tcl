#
# Copyright (c) 1993-1996 Regents of the University of California.
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
# @(#) $Header$ (LBL)
#

#
# destroy a viewing window but remember where it was
# and what size it was
#
proc destroy_userwin w {

	global win_src
	set src $win_src($w)

	switcher_unregister $w

	detach_window $src $w

	set x [winfo rootx $w]
	set y [winfo rooty $w]
	# adjust for virtual desktops
	incr x [winfo vrootx $w]
	incr y [winfo vrooty $w]
	set top [winfo toplevel $w]
	global userwin_x userwin_y userwin_size size$top
	set userwin_x($src) $x
	set userwin_y($src) $y
	set userwin_size($src) [set size$top]
	destroy $top
}

#
# resize a video window
#
proc resize { vw w h } {
	$vw resize $w $h
	global win_src
	set src $win_src($vw)
	detach_renderer $src $vw
	#
	# Force an update so the window gets mapped at
	# the new size before we attach the renderer.
	#
	update idletasks

	attach_renderer $src $vw
}

#
# true if w is a top-level viewing window
#
proc viewing_window w {
	if { [string range $w 0 2] == ".vw"} {
		return 1
	} else {
		return 0
	}
}

proc window_set_slow w {
	global win_target win_is_slow
	if $win_is_slow($w) {
		set interval [option get . stampInterval Vic]
	} else {
		set interval 0
	}
	$win_target($w) update-interval $interval
}

#
# delete and create a new renderer on window $w without
# changing anything else
#
proc reallocate_renderer w {
	global win_src
	set src $win_src($w)
	detach_window $src $w
	attach_window $src $w
}

#
# Create a video widget (as implemented in vw.cc) and initialize all the
# global state associated with a window.
#
proc create_video_widget { w width height } {

	video $w $width $height

	#
	# Use the window "name" as the variable for its mode
	#
	global win_is_switched win_is_timed win_is_slow win_use_hw V
	set win_is_switched($w) 0
	set win_is_timed($w) 0
	set win_is_slow($w) 0
#	set win_use_hw($w) $V(useHardwareDecode)
	if { $V(useHardwareDecode) } {
		set win_use_hw($w) "magic"
	} else {
		set win_use_hw($w) "software"
	}
}

proc HACK_detach_xil {} {
	global win_use_hw

	foreach xx [array names win_use_hw] {
		if { $win_use_hw($xx) == "xil" } {
			set win_use_hw($xx) "software"
			if [info exists win_src($xx) ] {
				reallocate_renderer $xx
			}
		}
	}
}

#
# create a new window for viewing video
#
proc open_window src {
	set f [smallfont]	
	set uid [uniqueID]
	set w .vw$uid
	toplevel $w -class Vic \
		-visual "[winfo visual .top] [winfo depth .top]" \
		-colormap .top
	catch "wm resizable $w false false"
	frame $w.frame


	global size$w userwin_x userwin_y userwin_size
	if [info exists userwin_x($src)] {
		wm geometry $w +$userwin_x($src)+$userwin_y($src)
		wm positionfrom $w user
		set size$w $userwin_size($src)
		set d [split $userwin_size($src) x]
		create_video_widget $w.frame.video [lindex $d 0] [lindex $d 1]
	} elseif [isCIF [rtp_format $src]] {
		create_video_widget $w.frame.video 352 288
		set size$w 352x288
	} else {
		create_video_widget $w.frame.video 320 240
		set size$w 320x240
	}
	set v $w.frame.video

	frame $w.bar
	button $w.bar.dismiss -text Dismiss -font $f -width 8 \
		-highlightthickness 0

	set m $w.bar.mode.menu
	menubutton $w.bar.mode -text Modes... -menu $m -relief raised \
		-width 8 -font $f
	menu $m

	$m add checkbutton -label Voice-switched \
		-command "window_set_switched $v" \
		-font $f -variable win_is_switched($v)
	$m add checkbutton -label Timer-switched \
		-command "window_set_timed $v" \
		-font $f -variable win_is_timed($v)
	$m add checkbutton -label Save-CPU \
		-command "window_set_slow $v" \
		-font $f -variable win_is_slow($v)

	if ![have cb] {
		$m entryconfigure Voice-switched -state disabled
	}

	set m $w.bar.size.menu
	menubutton $w.bar.size -text Size... -menu $m -relief raised -width 8 \
		-font $f
	menu $m
	$m add radiobutton -label QCIF -command "resize $v 176 144" \
		-font $f -value 176x144 -variable size$w
	$m add radiobutton -label CIF -command "resize $v 352 288" \
		-font $f -value 352x288 -variable size$w
	$m add radiobutton -label SCIF -command "resize $v 704 576" \
		-font $f -value 704x576 -variable size$w

	$m add separator
	$m add radiobutton -label "1/16 NTSC" \
		-command "resize $v 160 120" \
		-font $f -value 160x120 -variable size$w
	$m add radiobutton -label "1/4 NTSC" \
		-command "resize $v 320 240" \
		-font $f -value 320x240 -variable size$w
	$m add radiobutton -label NTSC \
		-command "resize $v 640 480" \
		-font $f -value 640x480 -variable size$w

	$m add separator
	$m add radiobutton -label "1/16 PAL" \
		-command "resize $v 192 144" \
		-font $f -value 192x144 -variable size$w
	$m add radiobutton -label "1/4 PAL" \
		-command "resize $v 384 288" \
		-font $f -value 384x288 -variable size$w
	$m add radiobutton -label PAL \
		-command "resize $v 768 576" \
		-font $f -value 768x576 -variable size$w


# Marcus ... 
	set m $w.bar.decoder.menu
	menubutton $w.bar.decoder -text Decoder... -menu $m -relief raised -width 8 -font $f
	menu $m
	$m add radiobutton -label Use-Magic \
		-command "reallocate_renderer $v" \
		-font $f -variable win_use_hw($v) -value magic
	
	global assistorlist

	if ![info exists assistorlist]  {
		set assistorlist [new assistorlist xx]
	}
	set d [$src handler]
	set fmt [rtp_format $src]
	if { $fmt == "jpeg" } {
		set fmt $fmt/[$d decimation]
	}
	set targets [$assistorlist assistors $fmt]
	foreach xname $targets {
		if { $xname != "" } {
			$m add radiobutton -label "Use-$xname-Assistor" \
				-command "reallocate_renderer $v" \
				-font $f -variable win_use_hw($v) -value $xname
		}
	}

	$m add radiobutton -label "Use-VIC Software" \
		-command "reallocate_renderer $v" \
		-font $f -variable win_use_hw($v) -value software

# ... Marcus

	label $w.bar.label -text "" -anchor w -relief raised
	pack $w.bar.label -expand 1 -side left -fill both
	pack $w.bar.decoder $w.bar.size $w.bar.mode $w.bar.dismiss -side left -fill y

	pack $w.frame.video -anchor c
	pack $w.frame -expand 1 -fill both
	pack $w.bar -fill x

	bind $w <Enter> { focus %W }
	#wm focusmodel $w active

	bind $w <d> "destroy_userwin $v"
	bind $w <q> "destroy_userwin $v"
	$w.bar.dismiss configure -command "destroy_userwin $v"

	bind $w <Return> "switcher_next $v"
	bind $w <space> "switcher_next $v"
	bind $w <greater> "switcher_next $v"
	bind $w <less> "switcher_prev $v"
	bind $w <comma> "switcher_prev $v"

	switcher_register $v $src window_switch

	#
	# Finally, bind the source to the window.
	#
	attach_window $src $v
	windowname $w [getid $src]
}

proc windowname { w name } {
	if ![yesno suppressUserName] {
		$w.bar.label configure -text $name
	}
	wm iconname $w vic:$name
	wm title $w $name
}

proc window_switch { w src } {
	global win_src
	if { [info exists win_src($w)] } {
		set oldsrc $win_src($w)
	} else {
		set oldsrc "lost"
	}
	if { $oldsrc != $src } {
		detach_window $oldsrc $w
		attach_window $src $w
	}
}

proc window_set_switched w {
	global win_is_switched
	if { $win_is_switched($w) != 0 } {
		switcher_enable $w
	} else {
		switcher_disable $w
	}
}

proc window_set_timed w {
	global win_is_timed
	if { $win_is_timed($w) != 0 } {
		switcher_set_timer $w
	} else {
		switcher_cancel_timer $w
	}
}
