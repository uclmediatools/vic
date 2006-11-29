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
	wm geometry $w $geo
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
proc open_window_frame { src width height id } {
	global PATH_
	set uid [uniqueID]
	set w .vv$uid
	frame $w
	#set w [frame .$uid]
	frame $w.layoutframe	
	create_video_widget $w.layoutframe.video $width $height		
	set v $w.layoutframe.video

	frame $w.bar -height 1 

#Morris...    Select List
	
	global active radio
    	set m $w.bar.list.menu
    	menubutton $w.bar.list -text Select -menu $m -state normal -relief raised -width 8 
    	menu $m
    	$m add cascade -label "Switch options..." -menu $m.opt
    	menu $m.opt -tearoff no
    	foreach s [session active] {
    		set name [$s sdes name]
    		if { $name != "" } {
    			$m.opt add radiobutton -label $name \
      	  			-variable radio($w) \
      	  			-command " window_switch $v $s "
      	  		set completed 1
      	  	} else {
	      	  	set completed 0
      		}
       	}
       	if { $completed != 0 } {
       		$w.bar.list configure -state normal
       	}
       	
#Morris....     Label - show name

  	global username_ stringlen
  	set username_($v) [$src sdes name]
  	if { $stringlen == 10 } {
  		set username_($v) [string range $username_($v) 0 8]
  	} elseif { $stringlen == 15 } {
  		set username_($v) [string range $username_($v) 0 15]
  	} else {
  		set username_($v) [$src sdes name]  
  	}
        label $w.bar.name -textvariable username_($v) -relief flat
# comment next line to remove buttons
	pack $w.bar.name -side left -expand 1 -fill x
	pack $w.bar.list -side left -fill y
	pack $w.bar -fill x
	
	pack $w.layoutframe.video -anchor c
	pack $w.layoutframe -expand 1 -fill both

	#
	# Finally, bind the source to the window.
	#
	attach_window $src $v
	return $w
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
		-visual "[winfo visual .top] [winfo depth .top]" \
		-colormap .top
	catch "wm resizable $w false false"
	#
	# make windows become x-y resizeable
	#
	#catch "wm resizable $w true true"
	frame $w.frame


	global size$w userwin_x userwin_y userwin_size
	if [info exists userwin_x($src)] {
		wm geometry $w +$userwin_x($src)+$userwin_y($src)
		wm positionfrom $w user
		set size$w $userwin_size($src)
		set d [split $userwin_size($src) x]
		create_video_widget $w.frame.video [lindex $d 0] [lindex $d 1]
	} else {
	   # show the video frame accroding to it's resolution
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

	# added to catch window close action
	wm protocol $w WM_DELETE_WINDOW "destroy_userwin $v"

	bind $w <Return> "switcher_next $v"
	bind $w <space> "switcher_next $v"
	bind $w <greater> "switcher_next $v"
	bind $w <less> "switcher_prev $v"
	bind $w <comma> "switcher_prev $v"
	# double clicking to toggle fullscreen mode
	bind $w <Double-1> {
	  set src $win_src(%W)
	  destroy_userwin %W
	  open_full_window $src	  
	}
	
	# Resize
	bind $w <B1-Motion> {	  
	  global win_src win_target

	  if { [info exists win_src(%W)] & [info exists win_target(%W)]} {
	    # %W is vw.frame.video
	    set src $win_src(%W)
	    
	    # return Decoder "d" as a PacketHandler
	    set d [$src handler]
	    
	    set iw [$d width]
	    set ih [$d height]

	    set aspect_r [expr 1.0*$ih / $iw]
	    
	    set ow [expr int(%w + %x +%y)]
	    set oh [expr int($aspect_r * $ow)]

 	    if { [expr abs(%x) + abs(%y)] > 10 && $ow > 64 } {
              # open_dialog "$iw $ih $ow $oh"
              resize %W $ow $oh		       	      
	      #resize_window %W $ow $oh   	      
	    }
	       
	  }	 
	}		
	switcher_register $v $src window_switch

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
		-visual "[winfo visual .top] [winfo depth .top]" \
		-colormap .top
	catch "wm resizable $w false false"

	# for bordless window	
	set sw_ [winfo screenwidth .]
	set sh_ [winfo screenheight .]
	set sw $sw_
	set sh $sh_

	puts "original fullscreen size: $sw $sh"
	if { $sh_ >= 1280 || $sw_ >= 1024} {
	  set sw 1280
	  set sh 1024	   
	} elseif {$sh_ >= 1024 || $sw_ >= 726} {
	  set sw 1024
	  set sh 768	
	} elseif {$sh_ >= 800 || $sw_ >= 600} {
	  set sw 800
	  set sh 600	
	} elseif {$sh_ >= 640 || $sw_ >= 480} {
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

	bind $w <Enter> { focus %W }

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

proc windowname { w name } {
	if ![yesno suppressUserName] {
		$w.bar.label configure -text $name
	}
	wm iconname $w vic:$name
	wm title $w $name
}

proc window_switch { w src } {
	global win_src interface username_

	if { [info exists win_src($w)] } {
		set oldsrc $win_src($w)
	} else {
		set oldsrc "lost"
	}
	
	#debug_display $win_src($w) $src
	set oldwin [return_layout_win $src]
	
	if { $oldsrc != $src } {
		detach_window $oldsrc $w
		attach_window $src $w
		if { $oldwin != "" } {
			detach_window $src $oldwin
   		    attach_window $oldsrc $oldwin

		}
	}
	
	if { $interface == 1 } {
	    set username_($w) [$src sdes name]
	    set username_($oldwin) [$oldsrc sdes name]
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

######################################################
##### Switch Video
######################################################
#
# Rebuild the switchable list menu.
# This menu gets updated whenever the name of a participant
# changes, or someone gets activated or deactivated.
#
proc rebuild_switch_list_menu { path_ } {
	
    set m $path_.bar.list.menu
    #set m $path_.bar.list
    if { ![winfo exists $m]} {
	    return ""
    }
    
    # destroy the old menu first
    global radio active src_listbox add_one

     	#To prevent empty video enter, reduce one error
    	if { $add_one == 0 } {
     		return
     	} else {
    	     	catch "destroy $m.opt"
     	} 
     	
     	#
    	# rebuild options menu
    	# button down a name only if it was clicked before
    	# remove a name if it is no longer there
    	#
	menu $m.opt -tearoff no
   	# debug_display $path_ $m
    	#Morris...    Select List
    	foreach s [session active] {
    		set name [$s sdes name]
    	  	if { $name != "" } {
      	  		 $m.opt add radiobutton -label $name  \
      	  			-variable radio($path_) \
      	  			-command " window_switch $path_.layoutframe.video $s "   	 
  	      	} else {
	      		set flag 1
	      	}      	  	
     	}
}
#################################################
## Rebuild user list on top of client streaming 
#################################################
proc rebuild_all_list_menu { } {
    global win_path count active src_listbox add_one 
    
    #for update user list in left window
    lappend src_listbox "temp"
    set src_listbox [lreplace $src_listbox 0 end]
    set list_size [.top.grid.listbox size]
    .top.grid.listbox delete 0 $list_size
  
    foreach s [session active] {
    	  set name [$s sdes name]
    	  if { $name != "" } {
      	  	lappend src_listbox $s
      	  	.top.grid.listbox insert end $name 
      	  	##for generate add_one value
      	  	set list_size [ expr $list_size - 1 ] 
     	 }
     }     	
    set add_one $list_size
    
    #rebuild all window list
    set count [array size active]
    foreach p [array names win_path] {
        rebuild_switch_list_menu $p
    }  
}

proc return_layout_win { src } {
    global win_src

    #debug_display [array size win_src] $src
    foreach win [array names win_src] {
        set value $win_src($win)       
        #debug_display [string f $win] $win
        if { [string first "layoutframe" $win] != -1 && [string compare $value $src] == 0 } {
            return $win
        } else {
        }
    }
    return "" 
}