#
# Copyright (c) 1998 University College London
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

# K.Hasler.  01/03/98
# Note: Procedures based on original VIC procedures are appended with 'relate'

# -------------------------------------------------
# relate_quit --
#	Setup the conference bus to receive a quit
#	message from Relate.  Calls adios function to 
#	quit VIC.
# -------------------------------------------------

set cb_dispatch(relate_quit) relate_quit
proc relate_quit msg {
	adios
}

# -------------------------------------------------
# relate_power --
#	Setup the conference bus to receive a power
#	message from RAT.  The message consists of a
#	cname and the power value.
# -------------------------------------------------

set cb_dispatch(relate_power) relate_power

proc relate_power msg {
	foreach s [session active] {
		set ipaddr [lindex $msg 0]
		if { [$s sdes cname] == $ipaddr } {
			service_audio_power $s [lindex $msg 1]
			return
		}
	}
}

# -------------------------------------------------
# service_auido_power --
#	Given a window source and a power level, first
#	find if this is a standard window or big window
#	and then configure 'bar' to adjusted level. 
# -------------------------------------------------

proc service_audio_power { src level } {
	global win_src V
	set w 0
	if {[info exists win_src(.top.big.frame.video)]} {
		set w $win_src(.top.big.frame.video)
	}
	if {$w!=$src} {
		set w $V(grid).$src.stamp.power.bar
		set bar_length 114
	} else {
		set w .top.big.frame.power.bar
		set bar_length 290
	}
  
  # calculate the absolute length of the 
  set length  [expr ($level * $bar_length) / 100];

  if ![winfo exists $w] {
     return
  }
  
  if {$length>$bar_length} {
      set length $bar_length
  }
  $w configure -height $length 
}

# -------------------------------------------------
# redocorate_relate --
#	Deactivate all sources currently active.  If a 
#	big window exists destroy that as well.  New 
#	video windows will be created as new packets 
#	arrive.  Updated is set to 1 to remove old 
#	stats update in 'update_src'
# -------------------------------------------------

proc redecorate_relate { } {
	global V window_no updated
	set updated 1
	if ![info exists V(grid)] {
		return
	}
	if {[winfo exists .top.big.frame]} {destroy_userwin_relate .top.big.frame}
	set w $V(grid)
	grid propagate $w 0
	set list [session active]
	foreach src $list {
		if {"$src"!="$V(session)"} {$src deactivate} 
	}
	repack_relate
}

# -------------------------------------------------
# rm_active_relate --
#	Remove video window from active list
# -------------------------------------------------

proc rm_active_relate src {
	global active V window_no
	set window_no [expr $window_no-1]
	unset active($src)
}

# -------------------------------------------------
# deactivate_relate --
#	Destroy video window and all related variables
#	ie stats.
# -------------------------------------------------

proc deactivate_relate src {
	global ftext btext ltext fpshat bpshat lhat shat win_list V 
	unset ftext($src)
		if [info exists win_list($src)] {
		foreach w $win_list($src) {
			if [viewing_window $w] {
				#XXX should check if we're voice-switched
				# and if so, bump window
				destroy_userwin_relate $w
			} else {
				# thumbnail - just detach 
				detach_window $src $w
			}
		}
	}

	set w $V(grid).$src
	if [winfo exists $w] {
		grid forget $w
		destroy $w
		rm_active_relate $src
	}
	destroy_decoder_stats $src
	destroy_rtp_stats $src
	delete [$src handler]

	unset btext($src)
	unset ltext($src)
	unset fpshat($src)
	unset bpshat($src)
	unset lhat($src)
	unset shat($src)
	after 500 {repack_relate}
}

# -------------------------------------------------
# rm_active_relate --
#	Remove big video window.
# -------------------------------------------------

proc destroy_userwin_relate w {
	global win_src
	if {[winfo exists $w.video]==1} {
		catch {
			set src $win_src($w.video)
			detach_window $src $w.video
		}
		destroy $w
	}
	repack_relate
}

# -------------------------------------------------
# add_active_relate --
#	Add a window source to the active list.
# -------------------------------------------------

proc add_active_relate src {
	global active
	set active($src) 1
}

# -------------------------------------------------
# build.powerbars --
#	Create powerbars, stats and username for window
# -------------------------------------------------

proc build.powerbars {stamp src} {
	set f [smallfont]
	# Create power bar
	frame $stamp.power -width 20 -relief groove -bg grey -bd 3

	pack $stamp -expand 1  -fill both
	frame $stamp.power.bar -width 10 -height 1 -bg green -relief flat
	pack $stamp.power.bar -side bottom 

	label $stamp.name -width 20 -textvariable src_nickname($src) -font $f 
	pack $stamp.name  -expand 1 -side top -fill x
	
	global ftext btext ltext
	set ftext($src) "0.0 f/s"
	set btext($src) "0.0 kb/s"
	set ltext($src) "(0%)"
	frame $stamp.bar
	label $stamp.bar.fps -textvariable ftext($src) -width 6 \
		-font $f -pady 0 -borderwidth 0
	label $stamp.bar.bps -textvariable btext($src) -width 8 \
		-font $f -pady 0 -borderwidth 0
	label $stamp.bar.loss -textvariable ltext($src) -width 6 \
		-font $f -pady 0 -borderwidth 0

	global mutebutton V
	set mutebutton($src) $V(muteNewSources)
	$src mute $mutebutton($src)

	checkbutton $stamp.bar.mute -bd 1 -relief flat  -text M -font $f -width 1\
	  -variable mutebutton($src) -command "$src mute \$mutebutton($src)"
	pack $stamp.bar.fps $stamp.bar.bps $stamp.bar.loss $stamp.bar.mute -side left -fill x -expand 1
	pack $stamp.bar -side bottom -fill x -expand 1
	pack $stamp.power -side right -fill y
}

# -------------------------------------------------
# open_window_relate --
#	Create a big video window with maximum update
#	of 6 fps.  Name is bound to procedure to 
#	destory window.
# -------------------------------------------------

proc open_window_relate src {
	set w .top.big
	frame $w.frame
	create_video_widget $w.frame.video 342 294
	global win_is_slow win_target
	build.powerbars $w.frame $src
	set v $w.frame.video
	pack $v -anchor c
	pack $v -expand 1 -fill both
	bind $w.frame.name <1> "destroy_userwin_relate $w.frame"

	#
	# Finally, bind the source to the window.
	#
	attach_window $src $v
	#set target $win_target($v) 
	#$target update-interval 6

}

# -------------------------------------------------
# select_thumbnail_relate --
#	Destroy current big window and create a new
#	one given a video source. 
# -------------------------------------------------

proc select_thumbnail_relate { w src } {
	global winstate
	if {$winstate==1} {
		set vw .top.big.frame
		if {[winfo exists $vw]==1} {
			destroy_userwin_relate $vw
		}
		pack forget $w.stamp
		open_window_relate $src
	}
}

# -------------------------------------------------
# really_activate_relate --
#	Remove big video window.
# -------------------------------------------------

proc really_activate_relate src {
	add_active_relate $src
	global V
	set w $V(grid).$src
	frame $w -relief groove -borderwidth 0 \
		-visual $V(visual) -colormap $V(colormap)
	#XXX color
	pack propagate $V(grid) 0
	grid propagate $V(grid) 0
	grid $w -row $V(currow) -column $V(curcol) -sticky nw
	grid columnconfigure $V(grid) $V(curcol) -weight 1
	grid rowconfigure $V(grid) $V(currow) -weight 1
	if {[build.src.relate $w $src 1]==1} {
		update_decoder $src
		after 4000 "global updated; set updated 0; catch {update_src $src}"
		bump
	}
}

# -------------------------------------------------
# repack_relate --
#	Pack video windows into a grid with two rows
#	and destroy big window.
# -------------------------------------------------

proc repack_relate { } {
	global V winstate window_no
	set V(curcol) 0
	set V(currow) 0
	set V(ncol) 2
	if ![info exists V(grid)] {
		return
	}
	set window_no 0
	if {[winfo exists .top.big.frame]} {destroy_userwin_relate .top.big.frame}
	set w $V(grid)
	grid propagate $w 0
	set list [session active]
	foreach src $list {
		incr window_no
		if {[expr $winstate == 1 && $window_no > 4]  || [expr $winstate == 0 && $window_no > 8]} {
		pack forget $w.$src.stamp
		} else {
		grid $w.$src -row $V(currow) -column $V(curcol) -sticky nw
		pack $w.$src.stamp
		}
		bump
	}
}

# -------------------------------------------------
# build.src.relate --
#	Create a new small window with max fps of 1.
#	If the maximum number of windows has been
#	reached then hide window.
# -------------------------------------------------

proc build.src.relate { w src color } {
	global winstate window_no tcl_platform
	incr window_no
	set stamp $w.stamp
	frame $stamp -relief ridge -borderwidth 2
	if {$tcl_platform(platform)=="windows"} {
		create_video_widget $stamp.video 159 120
	} else {
		create_video_widget $stamp.video 159 128
	}
	build.powerbars $stamp $src			
	bind $stamp.name <1> "select_thumbnail_relate $w $src"
	bind $stamp.video <Enter> { focus %W }

	#global win_is_slow
	#set win_is_slow($stamp.video) 1
	attach_window $src $stamp.video
	pack $stamp.video -anchor c
	if {[expr $winstate == 1 && $window_no > 4]  || [expr $winstate == 0 && $window_no > 8]} {
		pack forget $stamp
	} else {
		pack $stamp -side left
	}
	bind $stamp.video <d> "$src deactivate"
	return 1
}

# -------------------------------------------------
# build.bar.relate --
#	Create controls at bottom of window.
# -------------------------------------------------

proc build.bar.relate { w } {
	global title winstate
	frame $w.bar
	set winstate 0
	checkbutton $w.bar.send -font [smallfont] -text "Transmit" -borderwidth 2 \
		-relief raised -command transmit -variable transmitButtonState \
		-anchor w -state disabled

	after 1000 {
		global transmitButton
		if {[$transmitButton cget -state]=="normal"} {
			.top.bar.send configure -state normal
			return
		}
	}

	button $w.bar.release -font [smallfont] -text "Stop" -borderwidth 2 \
		-relief raised -command release_device

	menubutton $w.bar.mb -menu $w.bar.mb.m -font [smallfont] -text Participants -relief raised
	menu $w.bar.mb.m 
	$w.bar.mb.m add checkbutton -label "four" -command four_windows -variable winstate
	$w.bar.mb.m add checkbutton -label "eight" -command eight_windows -onvalue 0 -offvalue 1 -variable winstate

	label $w.bar.title -text "VIC [version]" -font [smallfont]
	button $w.bar.quit -text Quit -borderwidth 2 -relief raised \
	 -font [smallfont] -command adios
	button $w.bar.refresh -font [smallfont] -text "Refresh" -borderwidth 2 -relief raised -command redecorate_relate
	button $w.bar.menu -text Admin -borderwidth 2 -relief raised \
		-font [smallfont] \
		-command "toggle_window .menu"
	button $w.bar.help -text Help -borderwidth 2 -relief raised \
		-font [smallfont] \
		-command "toggle_window .help"

	# pack $w.bar.menu $w.bar.help $w.bar.quit -side left
	pack $w.bar.send $w.bar.release -side left
	pack $w.bar.title -side left -fill x -expand 1
	pack $w.bar.mb -side left -fill y
	pack $w.bar.refresh -side left
	pack $w.bar.menu -side left
}

# -------------------------------------------------
# four_windows --
#	Configure window to hold four windows and one
#	large window.
# -------------------------------------------------

proc four_windows {} {
global V winstate
set winstate 1
.top.big configure -height 344
$V(grid) configure -height 340
redecorate_relate
}

# -------------------------------------------------
# eight_windows --
#	Configure window to hold eight windows.
# -------------------------------------------------

proc eight_windows {} {
global V winstate
set winstate 0
.top.big configure -height 0
$V(grid) configure -height 684
repack_relate
}

# -------------------------------------------------
# build.new.interface --
#	Setup initial window
# -------------------------------------------------

proc build.new.interface {} {
	global V window_no
	init_visual .top
	if ![init_color] {
		if { [winfo depth .top] != 8 } {
			puts stderr "vic: internal error: no colors"
			exit 2
		}
		puts stderr \
		    "vic: warning: ran out of colors; using private colormap"
		destroy .top
		frame .top -visual $V(visual) -colormap new
		if ![init_color] {
			puts stderr "vic: internal error: no colors"
			exit 2
		}
	}
    
	build.srclist
	set window_no 0
	set_rate_vars $V(session)

	#
	# emulate implicit keyboard focus
	#
	#bind . <Enter> { focus %W }
	#wm focusmodel . active
	bind . <q> { adios }
	bind . <Control-c> { adios }
	bind . <Control-d> { adios }

	set V(curcol) 0
	set V(currow) 0
	set V(ncol) 2
	set V(muteNewSources) [yesno muteNewSources]
	set w .top
	.top configure -height 718 -width 362
	pack propagate .top 0
	set  V(grid) $w.thumbs
 
	wm title . "ReLaTe"

	frame $w.big -height 344 -width 362 -relief groove -bd 2
	frame $V(grid) -relief groove -bd 2 -height 340 -width 362
	build.bar.relate .top
	pack propagate $w.big 0
	pack $w.big -fill both -side top
	pack $V(grid) -expand 1 -fill both -side top
	pack .top.bar -fill x -side bottom
	pack .top -expand 1 -fill both -side bottom
	eight_windows
	after 1000 periodic_update
}
