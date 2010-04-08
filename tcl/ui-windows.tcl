#
# Copyright (c) 1993-1996 The Regents of the University of California.
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

#
# destroy a viewing window but remember where it was
# and what size it was
#
set server_socket ""
set button_active 0

proc destroy_userwin {w {bypass false} } {

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
	if { $bypass == "false" } {
	  global userwin_x userwin_y userwin_size size$top
	  set userwin_x($src) $x
	  set userwin_y($src) $y
	  set userwin_size($src) [set size$top]
	}
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

	attach_renderer $src $vw true
}

#
# fit video window size as soure resolution
#
proc fit_window { vw } {

	global win_src
	set src $win_src($vw)
		
	set d [$src handler]
	set iw [$d width]
	set ih [$d height]	

	resize $vw $iw $ih	
	resize_window $vw $iw $ih	
}

proc resize_window {vw width height} {

	set w [winfo parent [winfo parent $vw]]	
	set old_g [split [wm geometry $w] "x+"]
	set geo [format "%sx%s+%s+%s" $width $height [lindex $old_g 2] [lindex $old_g 3]]	
	#wm geometry $w $geo
	global size$w		
	set size$w [format "%sx%s" $width $height]
}
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
	attach_window $src $w true
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
	set d [$src handler]
	set iw [$d width]
	set ih [$d height]
	if { $iw == 0 || $ih == 0} {
	   puts "window width or height is zero"
	   return 
	}
	set w .vw$uid
	toplevel $w -class Vic \
		-visual "[winfo visual .top] [winfo depth .top]"
	catch "wm resizable $w false false"
	catch "$w configure -padx 0 -pady 0"
	#
	# make windows become x-y resizeable
	#
	#catch "wm resizable $w true false"
	frame $w.frame 
	catch "$w.frame configure -padx 0 -pady 0"

	global size$w userwin_x userwin_y userwin_size
	if [info exists userwin_x($src)] {
		wm geometry $w +$userwin_x($src)+$userwin_y($src)
		wm positionfrom $w user
		set size$w $userwin_size($src)
		if {$userwin_size($src) == "half"} {
			create_video_widget $w.frame.video [expr $iw / 2] [expr $ih / 2]
		} elseif {$userwin_size($src) == "original"} {
			create_video_widget $w.frame.video $iw $ih
		} elseif {$userwin_size($src) == "double"} {
			create_video_widget $w.frame.video [expr $iw * 2] [expr $ih * 2]
		} else {
			set d [split $userwin_size($src) x]
			create_video_widget $w.frame.video [lindex $d 0] [lindex $d 1]
		}
	} else {
		# show the video frame according to it's resolution
		create_video_widget $w.frame.video $iw $ih
		set size$w [format "%sx%s" $iw $ih]
	}
	#elseif [isCIF [rtp_format $src]] {
	#	create_video_widget $w.frame.video 352 288
	#	set size$w 352x288
	#} else {
	#	create_video_widget $w.frame.video 320 240
	#	set size$w 320x240
	#}
	set v $w.frame.video

	frame $w.bar
	button $w.bar.dismiss -text Dismiss -font $f -width 8 \
		-highlightthickness 0

	set m $w.bar.mode.menu
	menubutton $w.bar.mode -text Modes -menu $m -relief raised \
		-width 8 -font $f -indicatoron 1
	menu $m

        if {[string match [ windowingsystem] "aqua"]} { 
                menu $w.mbar -tearoff 0
                $w configure -menu $w.mbar
                $w.mbar add cascade -label File -menu $w.mbar.file
                menu $w.mbar.file -tearoff 0
                $w.mbar.file add command -label Close \
                        -accel Command-W -command "destroy_userwin $v"
                $w.mbar add cascade -label Modes -menu $m
        }
                 	
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
	menubutton $w.bar.size -text Size -menu $m -relief raised -width 8 \
		-font $f -indicatoron 1
	menu $m
        if {[string match [ windowingsystem] "aqua"]} {
                $w.mbar add cascade -label Size -menu $m
        }
	$m add radiobutton -label QCIF -command "resize $v 176 144" \
		-font $f -value 176x144 -variable size$w -accelerator "S"
	$m add radiobutton -label CIF -command "resize $v 352 288" \
		-font $f -value 352x288 -variable size$w -accelerator "M"
	$m add radiobutton -label SCIF -command "resize $v 704 576" \
		-font $f -value 704x576 -variable size$w -accelerator "L"

	$m add separator
	$m add radiobutton -label "1/16 NTSC" \
		-command "resize $v 160 120" \
		-font $f -value 160x120 -variable size$w -accelerator "Shift-S"
	$m add radiobutton -label "1/4 NTSC" \
		-command "resize $v 320 240" \
		-font $f -value 320x240 -variable size$w -accelerator "Shift-M"
	$m add radiobutton -label NTSC \
		-command "resize $v 640 480" \
		-font $f -value 640x480 -variable size$w -accelerator "Shift-L"

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

	$m add separator
	$m add radiobutton -label "Half size" \
		-command "resize $v [expr $iw / 2] [expr $ih / 2]" \
		-font $f -value "half" -variable size$w
	$m add radiobutton -label "Original size" -command "fit_window $v" \
		-font $f -value "original" -variable size$w -accelerator "O"
	$m add radiobutton -label "Double size" \
		-command "resize $v [expr $iw * 2] [expr $ih * 2]" \
		-font $f -value "double" -variable size$w

# Marcus ... 
	set m $w.bar.decoder.menu
	menubutton $w.bar.decoder -text Decoder -menu $m -relief raised -width 8 -font $f -indicatoron 1
        if {[string match [ windowingsystem] "aqua"]} {
                $w.mbar add cascade -label Decoder -menu $m
        }    	
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
# comment next line to remove buttons
#	pack $w.bar.decoder $w.bar.size $w.bar.mode $w.bar.dismiss -side left -fill y

	pack $w.frame.video -anchor c
	pack $w.frame -expand 1 -fill both
# comment next line to remove buttons
#	pack $w.bar -fill x

        if {![string match [ windowingsystem] "aqua"]} {
		$w.bar.size.menu add separator
		set m $w.bar.size.menu.decodercascade
		$w.bar.decoder.menu clone $m
		$w.bar.size.menu add cascade -menu $m -label Decoder
		set m $w.bar.size.menu.modecascade
		$w.bar.mode.menu clone $m
		$w.bar.size.menu add cascade -menu $m -label Mode
        }

	#bind $w <Enter> { focus %W }
	#wm focusmodel $w active

	bind $w <s> "resize $v 176 144; set size$w 176x144"
	bind $w <m> "resize $v 352 288; set size$w 352x288"
	bind $w <l> "resize $v 704 576; set size$w 704x576"
	bind $w <S> "resize $v 160 120; set size$w 160x120"
	bind $w <M> "resize $v 320 240; set size$w 320x240"
	bind $w <L> "resize $v 640 480; set size$w 640x480"
	bind $w <e> "resize $v 1000 750; set size$w 1000x750"
	bind $w <E> "resize $v 1024 768; set size$w 1024x768"
	bind $w <x> "resize $v 640 240; set size$w 640x240"
	bind $w <o> "fit_window $v; set size$w original"
	bind $w <z> "catch { wm overrideredirect $w 1; wm withdraw $w; wm deiconify $w}"
	bind $w <Z> "catch { wm overrideredirect $w 0; wm withdraw $w; wm deiconify $w}"


	bind $w <d> "destroy_userwin $v"
	bind $w <q> "destroy_userwin $v"
	$w.bar.dismiss configure -command "destroy_userwin $v"

	# added to catch window close action
	wm protocol $w WM_DELETE_WINDOW "destroy_userwin $v"
	$w.bar.dismiss configure -command "destroy_userwin $v"

	bind $w <Return> "switcher_next $v"
	bind $w <space> "switcher_next $v"
	bind $w <greater> "switcher_next $v"
	bind $w <less> "switcher_prev $v"
	bind $w <comma> "switcher_prev $v"

	bind $w <g> "set_window_glue $v 1"
	bind $w <G> "set_window_glue $v 0"
	bind $w <h> "set_hardware_render $v 1"
	bind $w <H> "set_hardware_render $v 0"

	switcher_register $v $src window_switch

	global window_glue
	set window_glue($v) 0
	global button_active vtk_client

	if {[string match [ windowingsystem] "aqua"]} {
	    bind $v <Button-2> {
		set w [lindex [split %W "."] 1]
		set m .$w.bar.size.menu
		tk_popup $m %X %Y
	    }
	    bind $v <Control-1> {
		set w [lindex [split %W "."] 1]
		set m .$w.bar.size.menu
		tk_popup $m %X %Y
	    }
	} else {
	    bind $v <Button-3> {
		set w [lindex [split %W "."] 1]
		set m .$w.bar.size.menu
		tk_popup $m %X %Y
	    }
	}

#	puts "w is $v"

	bind $v <Control-KeyPress> {
#	    puts "got ctl keypress %K %x %y"
	    if { [string length %K] == 1 } {
	  binary scan %K c keyval
	  send_to_vtk K 0 $keyval %x %y %W
	  break
	    }
	}
	bind $v <Button> {
	    global notifier_id ag_last_x ag_last_y

	    send_to_vtk D 0 %b %x %y %W

	    set button_active %b
	    set modifier 0

	    set ag_last_x %x
	    set ag_last_y %y

	    set notifier_id [after 100 ag_update_motion]
	}
	bind $v <Control-Button> {
	    global notifier_id
	    send_to_vtk D 0 [expr %b | 8] %x %y %W

	    set button_active %b
	    set notifier_id [after 100 ag_update_motion]
	}
	bind $v <Shift-Button> {
	    global notifier_id
	    send_to_vtk D 0 [expr %b | 16] %x %y %W

	    set button_active %b
	    set notifier_id [after 100 ag_update_motion]
	}
	bind $v <Shift-Control-Button> {
	    global notifier_id
	    send_to_vtk D 0 [expr %b | 8 | 16] %x %y %W

	    set button_active %b
	    set notifier_id [after 100 ag_update_motion]
	}

	bind $v <ButtonRelease> {
	    if $button_active {
	  global notifier_id
	  set button_active 0
	  after cancel $notifier_id
	  send_to_vtk U 0 %b %x %y %W
	    }
	}
	bind $v <Motion> {
	    if $button_active {
	  global ag_motion_x ag_motion_y ag_motion_W
#		send_to_vtk M 0 $button_active %x %y %W
	  set ag_motion_x %x
	  set ag_motion_y %y
	  set ag_motion_W %W
	    }
	}

	# double clicking to toggle fullscreen mode
	bind $w <Double-1> {
	  set src $win_src(%W)
	  destroy_userwin %W
	  open_full_window $src	  
	}
	
	# Resize
	bind $w <Meta-ButtonPress> {
	   global click_x click_y
	   set click_x %x
	   set click_y %y	
	}

        bind $w <Meta-ButtonRelease> { 
          global win_src win_target click_x click_y

          if { [info exists win_src(%W)] & [info exists win_target(%W)]} {
              # %W is vw.frame.video
              set src $win_src(%W)

              # iw/ih mean viewing video size rightnow
              set iw [%W width]
              set ih [%W height]

              set aspect_r [expr 1.0*$ih / $iw]
              set diff_x [expr %x - $click_x]
              set diff_y [expr %y - $click_y]

              set ow [expr int($iw + $diff_x + $diff_y)]
              set oh [expr int($aspect_r * $ow)]

              if { $ow > 64 } {
                 resize %W $ow $oh
                 #resize_window %W $ow $oh            
              }
          }
        }

	#switcher_register $v $src window_switch

	#
	# Finally, bind the source to the window.
	#
	attach_window $src $v true
	windowname $w [getid $src]
}

proc open_full_window src {
	
	set uid [uniqueID]
	set d [$src handler]
	set iw [$d width]
	set ih [$d height]
	if { $iw == 0 || $ih == 0} {
	   puts "window width or height is zero"
	   return 
	}
	set w .vw$uid
	toplevel $w -class Vic \
		-visual "[winfo visual .top] [winfo depth .top]" 
	catch "wm resizable $w false false"

	# for bordless window	
	set sw_ [winfo screenwidth .]
	set sh_ [winfo screenheight .]
	set sw $sw_
	set sh $sh_

	puts "original fullscreen size: $sw $sh"
	if { $sw_ >= 1280 || $sh_ >= 1024} {
	  set sw 1280
	  set sh 1024	   
	} elseif {$sw_ >= 1024 || $sh_ >= 726} {
	  set sw 1024
	  set sh 768	
	} elseif {$sw_ >= 800 || $s_ >= 600} {
	  set sw 800
	  set sh 600	
	} elseif {$sw_ >= 640 || $sh_ >= 480} {
          set sw 640
          set sh 480
        }
 
	puts "new fullscreen size: $sw $sh"

	wm overrideredirect $w true	

	frame $w.frame
	global size$w userwin_x userwin_y userwin_size
	create_video_widget $w.frame.video $sw $sh
	set size$w [format "%sx%s" $sw $sh]	
	set v $w.frame.video

	pack $w.frame.video -anchor c
	pack $w.frame -expand 1 -fill both

	#bind $w <Enter> { focus %W }

	bind $w <d> "destroy_userwin $v"
	bind $w <q> "destroy_userwin $v"

	# added to catch window close action
	wm protocol $w WM_DELETE_WINDOW "destroy_userwin $v"

	bind $w <Return> "switcher_next $v"
	bind $w <space> "switcher_next $v"
	bind $w <greater> "switcher_next $v"
	bind $w <less> "switcher_prev $v"
	bind $w <comma> "switcher_prev $v"
	# double clicking to toggle fullscreen mode
	bind $w <Double-1> {
	  destroy_userwin %W true
	  open_window $src  
	}
	
	switcher_register $v $src window_switch

	#
	# Finally, bind the source to the window.
	#
	attach_window $src $v true
	windowname $w [getid $src]
}

proc set_hardware_render {v setting} {
  global win_use_hw
  if { $win_use_hw($v) == "software" && $setting == 1} {
    set win_use_hw($v) "magic"
    reallocate_renderer $v
  } elseif { $setting == 0 } {
    set win_use_hw($v) "software"
    reallocate_renderer $v
  }
}
proc set_window_glue {v setting} {
  global window_glue
  set window_glue($v) $setting
}



proc ag_update_motion { } {
    global ag_motion_x ag_motion_y ag_motion_W
    global ag_last_x ag_last_y
    global button_active notifier_id

#    puts "update motion"

    if [info exists ag_motion_x] {

#    puts "$ag_last_x $ag_last_y $ag_motion_x $ag_motion_y"
	if {$ag_last_x != $ag_motion_x || $ag_last_y != $ag_motion_y} {
	    
	   send_to_vtk M 0 $button_active $ag_motion_x $ag_motion_y $ag_motion_W
	}
	set ag_last_x $ag_motion_x
	set ag_last_y $ag_motion_y	
    }

    set notifier_id [after 100 ag_update_motion]
}

proc init_vtk {} {
    set vtk_client ""
    global server_socket

    set server_socket ""

    set v [option get . vtkServer Vic]

    if { $v == "" } {
        set v "yukon.mcs.anl.gov/46352"
    }

    if { $v != "" } {
	set v1 [split $v "/"]
	set host [lindex $v1 0]
	set port [lindex $v1 1]

	puts "have host=$host port=$port"

	set server_socket ""
	catch { 
	    set server_socket [socket $host $port]
	    puts -nonewline $server_socket "VIC*"
	    flush $server_socket
	}

	if { $server_socket != "" } {
	    puts "connected to server $server_socket"
#	    fconfigure $server_socket -translation binary
	    fconfigure $server_socket -buffering none
	}
    }

#    set mysock [socket -server server_connect 10000]
#    puts "mysock is $mysock"
}

proc fsend_to_vtk { string } {
    global server_socket

    if { $server_socket != "" } {
        puts $server_socket $string
        flush $server_socket
    }
}

proc send_to_vtk_bin { type flags value x y window } {
    global server_socket

    if { $server_socket != ""} {
	puts "sending $type $value $x $y"
	set w [winfo width $window]
	set h [winfo height $window]
	set str [binary format a1cSSSSS $type 100 $value $x $y $w $h]
	set l [string length $str]
	binary scan $str c12 f
	puts "sending length $l $f"
	puts $server_socket $str
    }
}
proc send_to_vtk { type flags value x y window } {
    global server_socket

    if { $server_socket != ""} {
  puts "sending $type $value $x $y"
  set w [winfo width $window]
  set h [winfo height $window]
  set str "$type $value $x $y $w $h"
  set len [string length $str]
  puts -nonewline $server_socket [format "%03d%s" $len $str]
    }
}

proc server_connect { sock addr port } {
    puts "got server connect $sock $addr $port"
    global vtk_client
    set vtk_client $sock
    puts $vtk_client "hi there"
}

proc map_coordinates { x y window } {
    set wx [winfo width $window]
    set wy [winfo height $window]
    set mx [expr double($x) / double($wx)]
    set my [expr double($y) / double($wy)]
    return [list $mx $my]
}

proc destroy_from_wm vw {
    tk_dialog .destroy_dialog "Don't do that" "Press the 'd' key in the window to close a video window" "" 0 "OK"
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
