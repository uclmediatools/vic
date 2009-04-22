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

set updated 0

# Added adios procedure that closes logFrameRate file on exit
# then calls c++ routine to send RTCP bye message
#
proc adios {} {

	if {[yesno logFrameRate]} {
		global logFrameChannel
		close $logFrameChannel
	}
	# just call C++ based routine that sends a RTCP bye: SessionManager->send_bye()
	adios_rtcp
}

proc session args {
	global V
	eval $V(session) $args
}

proc build.bar w {

    global title

    frame $w.bar  -borderwidth 0
    if {[string match [ windowingsystem] "aqua"]} {
        global V
        set net $V(data-net)
	label $w.bar.title -text "Address: [$net addr]  Port: [$net port]  TTL: [$net ttl]" -font [smallfont] -justify left
        button $w.bar.quit -text Quit \
                -font [smallfont] \
                -command adios
        button $w.bar.menu -text Menu  \
                -font [smallfont] \
                -command "toggle_window .menu"
        button $w.bar.help -text Help \
                -font [smallfont] \
                -command "toggle_window .help"
    	button $w.bar.autoplace -text Autoplace \
				-font [smallfont] \
				-command "ag_autoplace::show_ui"
    } else {
        global V
        set net $V(data-net)
        label $w.bar.title -text "Address: [$net addr]  Port: [$net port]  TTL: [$net ttl]" -font [smallfont] -relief flat -justify left
        button $w.bar.quit -text Quit -relief raised \
                -font [smallfont] -command adios \
                -highlightthickness 1
        button $w.bar.menu -text Menu -relief raised \
                -font [smallfont] -highlightthickness 1 \
                -command "toggle_window .menu"
        button $w.bar.help -text Help -relief raised \
                -font [smallfont] -highlightthickness 1 \
                -command "toggle_window .help"
	    button $w.bar.autoplace -text Autoplace -relief raised  \
				-font [smallfont] -highlightthickness 1 \
				-command "ag_autoplace::show_ui"
    }                                
    pack $w.bar.title -side left -fill both -expand 1
    pack $w.bar.menu $w.bar.autoplace $w.bar.help $w.bar.quit -side left -padx 1 -pady 1 
}

proc build.bar2 w {

	frame $w.bar2 -relief ridge -borderwidth 0

        button $w.bar2.autoplace -text Autoplace -relief raised  \
		-font [smallfont] -highlightthickness 1 \
		-command "ag_autoplace::show_ui"
       button $w.bar2.pixrate -text Pixrate -relief raised \
		-font [smallfont] -highlightthickness 1 \
		-command "create_pixrate_stats_window"


	pack $w.bar2.autoplace $w.bar2.pixrate -side right -padx 1 -pady 1
}

#
# Look through the list of available visuals, and choose
# the best one that vic supports.
#
proc lookup_visual {} {
	set vlist [winfo visualsavailable .]
	if { [lsearch -exact $vlist "truecolor 24"] >= 0 || \
		 [lsearch -exact $vlist "truecolor 32"] >= 0 } {
		set visual "truecolor 24"
	} elseif { [lsearch -exact $vlist "truecolor 16"] >= 0 } {
		set visual "truecolor 16"
	} elseif { [lsearch -exact $vlist "pseudocolor 8"] >= 0 } {
		set visual "pseudocolor 8"
	} elseif { [lsearch -exact $vlist "staticgray 1"] >= 0 } {
		set visual "staticgray 1"
	} else {
		puts stderr "vic: no support for your display type"
		exit 1
	}
}

#
# initialize the visual, colormap, and dither global variables
# based on  X resources, available visuals, etc.  Create the
# specified window as a frame widget and set its visual.  We pick
# the visual only once and startup -- the user cannot change it
# interactively.  The user can change the dithering algorithms
# interactively constrained to the selected visual
# Sets the colormap global variable to $w since this window
# will always have the reference colormap.
#
proc init_visual w {
	global V

	set V(colormap) $w
	set dither [option get . dither Vic]
	if { $dither == "best" } {
		set dither ed
	}    
	set V(gamma) [option get . gamma Vic]
	if { $dither == "dither" } {
		set dither od
	}
	if { [lsearch -exact "od ed gray quantize" $dither] < 0 } {
		puts stderr "vic: unknown dither: $dither"
		exit 1
	}
	set visual [option get . visual Vic]
	if { $visual == "" } {
		set visual [lookup_visual]
	} elseif { $visual == "pseudocolor" } {
		set visual "pseudocolor 8"
	}
	set cmap ""
	if [yesno privateColormap] {
		set cmap "-colormap new"
	}
	if [catch "frame $w -visual {$visual} $cmap"] {
		puts stderr "$V(app): bad visual: $visual"
		exit 1
	}
	if { [winfo depth $w] == 8 } {
		set V(dither) $dither
	}
	set V(visual) $visual
}

#
# initialize the colormodel based on the visual, dither, and colormap
# global variables.  if we run out of colors, free the colormodel
# and return false (leaving it up to the caller to fix things)
# otherwise, return true.  Set the global variable colorModel
# to the corresponding tcl object.
#
proc init_color {} {
	global V
	if [info exists V(colorModel)] {
		delete $V(colorModel)
		unset V(colorModel)
	}
	set v [winfo visual $V(colormap)]
	set d [winfo depth $V(colormap)]
	if { $d == 8 } {
		set id $v/$d/$V(dither)
	} else {
		set id $v/$d
	}
	set cm [new colormodel $id]
	if { $cm == "" } {
		puts stderr "vic: unsupported visual type: $v"
		exit 1
	}
	$cm visual $V(colormap)
	$cm gamma $V(gamma)
	#XXX revert to gray?
	if ![$cm alloc-colors] {
		delete $cm
		return 0
	}
	set V(colorModel) $cm
	return 1
}

#
# Build the user-interface.
#
proc init_gui {} {
	global V
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

	set_rate_vars $V(session)

	#
	# emulate implicit keyboard focus
	#
	bind . <Enter> { focus %W }
	#wm focusmodel . active
	bind . <q> { adios }
	bind . <Control-c> { adios }
	bind . <Control-d> { adios }

	foreach i { 1 2 3 4 5 6 7 8} {
		bind . <Key-$i> "redecorate $i"
	}

        frame .top.barholder -relief ridge -borderwidth 2

        build.bar .top.barholder
#       build.bar2 .top.barholder

        pack .top.barholder.bar -fill x -side bottom
#       pack .top.barholder.bar2 -fill x -side bottom
        pack .top.barholder -side bottom -fill x
	pack .top -expand 1 -fill both

        label .top.label -text "Waiting for video..."
	pack .top.label -before .top.barholder -anchor c -expand 1

	#
	# Withdraw window so that user-placement is deferred
	# until after initial geometry is computed
	#
	set geom [option get . geometry Vic]
	wm withdraw .
	# added to catch window close action
	wm protocol . WM_DELETE_WINDOW adios
	wm geometry . $geom
	update idletasks
	set minwidth [winfo reqwidth .]
	set minheight [winfo reqheight .]
	#XXX
	if { $minwidth < 200 } {
		set minwidth 200
	}
	if { $minheight < 100 } {
		set minheight 100
	}
	wm minsize . $minwidth $minheight
	wm deiconify .

	set V(curcol) 0
	set V(currow) 0
	set V(ncol) [option get . tile Vic]

	set V(muteNewSources) [yesno muteNewSources]

	after 1000 periodic_update
}

proc add_active src {
	global active
	set active($src) 1
	if { [array size active] == 1 } {
		pack forget .top.label

		set w .top.grid
		global V
		set V(grid) $w
		frame $w
		pack $w -fill both -anchor n
	}
	invoke_source_callback activate $src
}

proc rm_active src {
	global active V
	unset active($src)
        invoke_source_callback deactivate $src
	if { ![yesno relateInterface] && [array size active] == 0 } {
		pack forget $V(grid)
		destroy $V(grid)
		pack .top.label -before .top.barholder -anchor c -expand 1
	}
}

proc periodic_update { } {
	if [have grabber] {
		global V
		update_rate $V(session)
	}
	update idletasks
	after 1000 periodic_update
}

proc set_rate_vars src {
	global fpshat bpshat lhat shat
	if [info exists fpshat($src)] {
		unset fpshat($src)
		unset bpshat($src)
		unset lhat($src)
		unset shat($src)
	}
	set gain [option get . filterGain Vic]
	set fpshat($src) 0
	rate_variable fpshat($src) $gain
	set bpshat($src) 0
	rate_variable bpshat($src) $gain
	set lhat($src) 0
	rate_variable lhat($src) $gain
	set shat($src) 0
	rate_variable shat($src) $gain

#XXX set guys in stat window too!
}
	
#
# Called when use clicks on thumbnail video window.
# Create a new window only if the window already
# isn't being displayed (in locked mode).  In this
# case, delete the existing window (i.e., clicking
# on the thumbnail gives a toggle action, but not
# for voice-switched or browse-mode windows).
#
proc select_thumbnail { w src } {
	global win_src
	set srcid [$src srcid]
	foreach w [winfo children .] {
		# this is fragile
		set vw $w.frame.video
		if { [winfo exists $vw] && $srcid == [$win_src($vw) srcid] && \
			![is_switched $vw] } {
			destroy_userwin $vw
			return
		}
	}
	open_window $src
}

proc update_source_info src {
	global src_info src_nickname src_name numLayers numDecoderLayers
	#
	# Figure out best presentation from available information.
	#
	set name [$src sdes name]
	set cname [$src sdes cname]
	set addr [$src addr]
	set fmt [rtp_format $src]
	if { $fmt == "pvh" } {
		if [info exists numDecoderLayers($src)] {
			set fmt $fmt\($numDecoderLayers($src)\)
		} else {
			set fmt $fmt\($numLayers\)
		}
	}
	if { $name == "" } {
		if { $cname == "" } {
			set src_nickname($src) $addr
			set info $addr/$fmt

		} else {
			set src_nickname($src) $cname
			set info "$addr/$fmt"

	invoke_source_callback sdes_update $src
		}
	} elseif [cname_redundant $name $cname] {
		set src_nickname($src) $name
		set info $addr/$fmt
	} else {
		set src_nickname($src) $name
		set info $cname/$fmt
	}
		set src_info($src) $cname/$fmt

	set msg [$src sdes note]
	if { $msg != "" } {
		set info $msg
	}
	set src_info($src) $info
	
	# only call change_name when name really changes
	if { ![info exists src_name($src)] || "$src_name($src)" != "$name" } {
		set src_name($src) $name
		change_name $src
	}
}

#
# This is a hack to get rid of ugly borer when displaying CIF sized
# images in a NTSC sized windows.
proc adjust_voff { win d } {
	set ow [$win width]
	set oh [$win height]
	set iw [$d width]
	set ih [$d height]
	$win voff 0
	if { $ow == 320 && $oh == 240 } {
		if { $iw == 352 && $ih == 288 } {
			$win voff 8
		} elseif { $iw == 176 && $ih == 144 } {
		}
	} elseif { $ow == 640 && $oh == 480 } {
		if { $iw == 352 && $ih == 288 } {
			$win voff 16
		}
	}
}

#
# create a renderer for window $w and attach it to $src
#
proc attach_renderer { src w {enable_xv false}} {
	global win_target win_is_slow win_use_hw V
	set d [$src handler]
	set target ""
	if { ! [info exists win_use_hw($w) ] } {
		set win_use_hw($w) "software"
	}
	if { $win_use_hw($w) == "magic" } {
		set fmt [rtp_format $src]
		if { $fmt == "jpeg" } {
			set fmt $fmt/[$d decimation]
		}
		set target [new assistor $fmt]
		if { $target != "" } {
			$target window $w
		} else {
			set win_use_hw($w) software
		}
	} else { if { $win_use_hw($w) != "software" } {
		set fmt [rtp_format $src]
		if { $fmt == "jpeg" } {
			set fmt $fmt/[$d decimation]
		}
		set target [new assistor $win_use_hw($w)/$fmt]
		if { $target != "" } {
			$target window $w
		} else {
			set win_use_hw($w) software
		}
	}}

	if { $target == "" } {
		set target [$V(colorModel) renderer $w [$d decimation]]
	}
	if $win_is_slow($w) {
		$target update-interval [option get . stampInterval Vic]
	}
	$target enable_xv $enable_xv
	adjust_voff $w $d
	$d attach $target
	set win_target($w) $target
}

proc detach_renderer { src w } {
	global win_target
	set d [$src handler]
	$d detach $win_target($w)
	delete $win_target($w)
	unset win_target($w)
}

#
# bind a source to a window so that the video stream from
# source $src appears in window $w
#
proc attach_window { src w {enable_xv false}} {
	attach_renderer $src $w $enable_xv
	global win_src win_list
	set win_src($w) $src
	lappend win_list($src) $w
	if [viewing_window $w] {
		windowname [winfo toplevel $w] [getid $src]
	}
}

proc detach_window { src w } {
	global win_src win_list
	detach_renderer $src $w
	unset win_src($w)
	# there must be an easier way to do this
	set k [lsearch -exact $win_list($src) $w]
	if { $k < 0 } {
		puts "vic: detach_window: XXX"
		exit 1
	}
	set win_list($src) [lreplace $win_list($src) $k $k]
}

proc window_highlight { w color } {
	if { $w != "" } {
		$w configure -background $color
		foreach child [winfo children $w] {
			window_highlight $child $color
		}
	}
}

proc set_background { w color } {
	$w configure -background $color
}

proc build.src { w src color } {
	set f [smallfont]
	set stamp $w.stamp
	frame $stamp -relief ridge -borderwidth 2

	create_video_widget $stamp.video 80 60
	global win_is_slow
	set win_is_slow($stamp.video) 1

	# disable xvideo fro stamp video
	attach_window $src $stamp.video false 

	if {[string match [ windowingsystem] "aqua"]} {
                pack $stamp.video -side left -padx 2 -pady 2
                pack $stamp -side left -anchor nw -padx {4 2} -pady 2
                frame $w.r -padx 2
	} else {
                pack $stamp.video -side left -anchor c -padx 2
                pack $stamp -side left -fill y
                frame $w.r
	}

	global V
# Show sender window as raised
	if { $src == [srctab local] } {
	  frame $w.r.cw -relief groove -borderwidth 2 -bg gray20
	} else {
	  frame $w.r.cw -relief groove -borderwidth 2
	}

	pack $w.r.cw -side left -expand 1 -fill both -anchor w -padx 0

	if {[string match [ windowingsystem] "aqua"]} {
                label $w.r.cw.name -textvariable src_nickname($src) -font $f \
                        -padx 2 -pady 1 -borderwidth 0 -anchor w
                label $w.r.cw.addr -textvariable src_info($src) -font $f \
                        -padx 2 -pady 1 -borderwidth 0 -anchor w
	} else {
                label $w.r.cw.name -textvariable src_nickname($src) -font $f \
                        -pady 1 -borderwidth 0 -anchor w
                label $w.r.cw.addr -textvariable src_info($src) -font $f \
                        -pady 1 -borderwidth 0 -anchor w
	}          


	global ftext btext ltext
	set ftext($src) "0.0 f/s"
	set btext($src) "0.0 kb/s"
	set ltext($src) "(0%)"
	frame $w.r.cw.rateinfo
	label $w.r.cw.rateinfo.fps -textvariable ftext($src) -width 6 \
		-font $f -pady 0 -borderwidth 0
	label $w.r.cw.rateinfo.bps -textvariable btext($src) -width 8 \
		-font $f -pady 0 -borderwidth 0
	label $w.r.cw.rateinfo.loss -textvariable ltext($src) -width 6 \
		-font $f -pady 0 -borderwidth 0


	frame $w.r.ctrl -borderwidth 0

	global mutebutton V
	set mutebutton($src) $V(muteNewSources)
	$src mute $mutebutton($src)

        if {[string match [ windowingsystem] "aqua"]} {
                checkbutton $w.r.ctrl.mute -text mute -borderwidth 2 \
                        -font $f -width 4 \
                        -command "$src mute \$mutebutton($src)" \
                        -variable mutebutton($src)

                checkbutton $w.r.ctrl.color -text color -borderwidth 2 \
                        -font $f -width 4 \
                        -command "\[$src handler\] color \$colorbutton($src)" \
                        -variable colorbutton($src)
        } else {
                checkbutton $w.r.ctrl.mute -text mute -borderwidth 2 \
                        -highlightthickness 1 \
                        -relief groove -font $f -width 4 \
                        -command "$src mute \$mutebutton($src)" \
                        -variable mutebutton($src)

                checkbutton $w.r.ctrl.color -text color -borderwidth 2 \
                        -highlightthickness 1 \
                        -relief groove -font $f -width 4 \
                        -command "\[$src handler\] color \$colorbutton($src)" \
                        -variable colorbutton($src)
        }
             
	set m $w.r.ctrl.info.menu$src
        if {[string match [ windowingsystem] "aqua"]} {
                menubutton $w.r.ctrl.info -text info -borderwidth 2 \
                        -font $f -pady 4 -menu $m
        } else {
                menubutton $w.r.ctrl.info -text info -borderwidth 2 \
                        -highlightthickness 1 \
                        -relief groove -font $f -width 5 \
                        -menu $m -indicatoron 1
        }      
	build_info_menu $src $m

        if {[string match [ windowingsystem] "aqua"]} {
                pack $w.r.ctrl.mute -side left -expand 1
                pack $w.r.ctrl.color -side left -expand 1
                pack $w.r.ctrl.info -side left -fill x -expand 1
        } else {
                pack $w.r.ctrl.mute -side left -fill x -expand 1
                pack $w.r.ctrl.color -side left -fill x -expand 1
                pack $w.r.ctrl.info -side left -fill x -expand 1
#               pack $w.r.ctrl.options -side left -fill x -expand 1
        } 

	global colorbutton
	set colorbutton($src) 1

	pack $w.r.cw.rateinfo.fps $w.r.cw.rateinfo.bps $w.r.cw.rateinfo.loss \
		-side left -anchor w
	pack $w.r.cw.name $w.r.cw.addr $w.r.cw.rateinfo -anchor w -fill x

	pack $w.r.cw -fill x -side top
	pack $w.r.ctrl -fill x -side top
	pack $w.r -side left -expand 1 -fill x

	if {[string match [ windowingsystem] "aqua"]} {
		bind $stamp <Enter> "%W configure -background CornflowerBlue"
		bind $stamp <Leave> "%W configure -background [resource background]"
	} elseif {$::tk_version > 8.4} {
		bind $stamp <Enter> "%W configure -background [$m cget -activebackground]"
		bind $stamp <Leave> "%W configure -background [$m cget -background]"
	} else {
		bind $stamp <Enter> "%W configure -background gray90"
		bind $stamp <Leave> "%W configure -background [resource background]"
	}

	bind $stamp.video <1> "select_thumbnail $w $src"
	bind $stamp.video <Enter> { focus %W }
	bind $stamp.video <d> "$src deactivate"

	return $stamp.video
}

proc bump { } {
	global V
	incr V(curcol)
	if { $V(curcol) == $V(ncol) } {
		set V(curcol) 0
		incr V(currow)
	}
}

proc redecorate n {
	global V
	set V(curcol) 0
	set V(currow) 0
	set V(ncol) $n

	if ![info exists V(grid)] {
		return
	}
	set w $V(grid)
	set list [session active]
	foreach src $list {
		grid $w.$src -row $V(currow) -column $V(curcol) -sticky we
		grid columnconfigure $w $V(curcol) -weight 1
		bump
	}
}

proc create_decoder src {
	global numLayers

	set format [rtp_format $src]
	set decoder [new decoder $format]
	if { $decoder == "" } {
		# don't support this format
		set decoder [new decoder null]
	}
##LL
	if { $format == "pvh" } {
		$decoder maxChannel $numLayers
	}
	$src handler $decoder
	return $decoder
}

proc set_busy src {}

#
# Add a src to the active senders list.  E.g., make a postage
# stamp window appear, stats, etc. so that the user can select
# the video stream.
#
proc activate src {
	global V

	if [yesno relateInterface] {
		set V(decoder) [create_decoder $src]
		after idle "really_activate_relate $src"
	} else {
		set V(decoder) [create_decoder $src]
	#
	# give decoder a chance see a packet so it can
	# determine the output geometry and color decimation.
	# we shouldn't have to do this (e.g., resize will
	# take care of a geomtry change), but currently
	# decoders can't trigger a renderer realloation
	# when the decimation changes.XXX fix this
	#
		after idle "really_activate $src"
	}
}

proc really_activate src {

	add_active $src

	global V
	set w $V(grid).$src
	frame $w -relief groove -borderwidth 0 \
		-visual $V(visual)
	#XXX color
	build.src $w $src 1
	grid $w -row $V(currow) -column $V(curcol) -sticky we
	grid columnconfigure $V(grid) $V(curcol) -weight 1

	update_decoder $src
	after 1000 "update_src $src"

	bump
}

proc update_decoder src {
	set_rate_vars $src
	update_source_info $src

	# check for Decoder control window
	set w .decoder_control$src
	if [winfo exists $w] {
#		destroy $w	

		#update
		set fmt [rtp_format $src]
		if [winfo exists $w.tb] {

			if { $fmt != "pvh" } { 
				pack forget $w.tb
			} else {
				pack $w.tb -before $w.dismiss  -fill x
			}
		} else {
			if { $fmt == "pvh" } { 

				global numDecoderLayers numLayers decoderLayerValue

				frame $w.tb
				set f [smallfont]

				if ![info exists numDecoderLayers($src)] {
					set numDecoderLayers($src) $numLayers
				}

				label $w.tb.value -text 0 -font $f -width 3
				scale $w.tb.scale -font $f -orient horizontal \
					-showvalue 0 -from 0 -to $numLayers \
					-variable numDecoderLayers($src) \
					-width 12 -relief groove \
					-command "set_numDecoderLayers $src"


				set decoderLayerValue($src) $w.tb.value

				$decoderLayerValue($src) configure -text $numDecoderLayers($src)

				pack $w.tb.scale -side left -fill x -expand 1
				pack $w.tb.value -side left
				pack $w.tb -before $w.dismiss  -fill x
				#create_decoder_control_window $src
			}
		}
	}

}

proc change_format src {
	global win_list

	if ![info exists win_list($src)] {
		#
		# if we get a change format before really_activate
		# was called (i.e., so we don't even have a thumbnail yet),
		# don't do anything
		#
		return
	}    
	set L $win_list($src)
	detach_renderers $src
	set extoutList [extout_detach_src $src]
	set d [$src handler]
	delete $d
	set d [create_decoder $src]
	update_decoder $src
	global colorbutton
	$d color $colorbutton($src)

	foreach w $L {
		attach_renderer $src $w
		$w redraw
	}
	extout_attach_src $src $extoutList
}

#
# Called when the video stream state changes in a way that would
# affect the choice of renderer.  For example, when a jpeg stream
# changes from type-0 to type-1 we might have to revert from
# hardware to software decoding, or we might have to reallocate
# a 422 renderer as a 420 renderer.  This never needs to happen
# for most stream types (i.e., because the decimation factor is fixed).
#
proc decoder_changed d {
	foreach src [session active] {
		if { [$src handler] == "$d" } {
			global active
			if ![info exists active($src)] {
				return
			}
			#XXX redundant with change_format
			global win_list
			set L $win_list($src)
			detach_renderers $src
			set extoutList [extout_detach_src $src]
			foreach w $L {
				attach_renderer $src $w
				$w redraw
			}
			extout_attach_src $src $extoutList

			return
		}
	}
}

#XXX
proc detach_renderers src {
	global win_list
	foreach w $win_list($src) {
		detach_renderer $src $w
	}
}

proc change_name src {
	global win_list nametag srclist
	set name [$src sdes name]
	# update viewing window names to reflect new name
	if [info exists win_list($src)] {
		foreach w $win_list($src) {
			if [viewing_window $w] {
				windowname [winfo toplevel $w] $name
			}
		}
	}
	# change name in source list if it exists
	if [info exists srclist] {
		$srclist itemconfigure $nametag($src) -text $name
	}
}

#
# Remove a src from the active senders list. 
#
proc deactivate src {
	global ftext btext ltext fpshat bpshat lhat shat win_list V

	#catch this if using relate interface
	if [yesno relateInterface] {deactivate_relate $src} else {
	if [info exists win_list($src)] {
		foreach w $win_list($src) {
			if [viewing_window $w] {
				#XXX should check if we're voice-switched
				# and if so, bump window
				destroy_userwin $w
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
		rm_active $src
	}
	destroy_decoder_stats $src

	delete [$src handler]

	unset ftext($src)
	unset btext($src)
	unset ltext($src)
	unset fpshat($src)
	unset bpshat($src)
	unset lhat($src)
	unset shat($src)
	}
}

proc update_rate src {
	global ftext btext ltext fpshat bpshat lhat shat V

	set key $src
	if { "$src" != "$V(session)" } {
		# only compute loss statistic for network side
		set p [$src np]
		set s [$src ns]
		set shat($key) $s
		set lhat($key) [expr $s-$p]
		if {$shat($key) <= 0.} {
			set loss 0
		} else {
			set loss [expr 100*$lhat($key)/$shat($key)]
		}
		if {$loss < .1} {
			set ltext($key) (0%)
		} elseif {$loss < 9.9} {
			set ltext($key) [format "(%.1f%%)" $loss]
		} else {
			set ltext($key) [format "(%.0f%%)" $loss]
		}
	}
	set bpshat($key) [$src nb]
	set fpshat($key) [$src nf]

	set fps $fpshat($key)
	set bps $bpshat($key)
	
	# csp
	catch {
	if {[resource logFrameRate] == "true"} {
		global logFrameChannel
		puts $logFrameChannel "[$src sdes cname] [clock seconds] $fps $bps $loss"
	}
	}

	if { $fps < .1 } {
		set fps "0 f/s"
	} elseif { $fps < 10 } {
		set fps [format "%.1f f/s" $fps]
	} else {
		set fps [format "%2.0f f/s" $fps]
	}
	if { $bps < 1 } {
		set bps "0 bps"
	} elseif { $bps < 1000 } {
		set bps [format "%3.0f bps" $bps]
	} elseif { $bps < 1000000 } {
		set bps [format "%3.0f kb/s" [expr $bps / 1000]]
	} else {
		set bps [format "%.1f Mb/s" [expr $bps / 1000000]]
	}
	set ftext($key) $fps
	set btext($key) $bps
}

proc update_src src {
	global ftext updated

	if ![info exists ftext($src)] {
		return
	}
	if {$updated==1} {return}
	update_rate $src
	#XXX
	update_source_info $src
	after 1000 "catch {update_src $src}"
}

proc build.help { } {
	create_help_window .help {
"Transmit video by clicking on the ``Transmit'' button \
in the ``Menu'' window.  You need video capture hardware to do this."

"Incoming video streams appear in the main vic window.  \
If you see the message ``Waiting for video...'', then no one is transmitting \
video to the conference address you're running on.  Otherwise, you'll \
see a thumbnail sized image and accompanying information for each source. \
Click on the thumbnail to open a larger viewing window.  You can tile the \
thumbnails in multiple columns using the ``Tile'' menu in the ``Menu'' window, \
or by pressing a number key (e.g., press 3 to view three columns)."
"Clicking on the ``mute'' button for a given source will \
turn off decoding.  It is usually a good idea to do \
this for your own, looped-back transmission."

"The transmission rate is controlled with the bit-rate \
and frame-rate sliders in the ``Transmission'' panel of the ``Menu'' window.  \
The more restrictive setting limits the transmission rate."

"The video windows need not be fixed to a given source. \
The ``Mode...'' menu attached to a viewing window allows you to specify \
voice-switched and/or timer-switched modes.   In timer-switched mode, the \
window automatically cycles through (unmuted) sources, while in \
oice-switched mode, the window switches to whomever is talking \
(using cues from vat).  You can have more than one voice-switched window, \
which results in a simple LRU allocation of the windows to most recent \
speakers.  See the man page for more details."

"If the user interface looks peculiar, you might \
have X resources that conflict with tk.  A common problem is \
defining ``*background'' and/or ``*foreground''."

"Bugs and suggestions to vic@cs.ucl.ac.uk  Thanks."
	}
}

#
# what we want printed in the info window for our format etc.
# (i.e., info window code is app independent but this info isn't,
#  so we have this hackish callback)
#
proc info_text src {
	set d [$src handler]
	set fmt [rtp_format $src]
	if { "$d" != "" } {
		    set fmt "$fmt [$d info] ([$d width]x[$d height])"
	}
	return "$fmt"
}
