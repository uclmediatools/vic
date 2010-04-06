
#
# Callback list for source registry/unregistry
#

proc ag_stdout_init_window {} {
    toplevel .stdout
    text .stdout.text -yscrollcommand ".stdout.scroll set"

    scrollbar .stdout.scroll -command ".stdout.text yview"
    pack .stdout.scroll -side right -fill y
    pack .stdout.text -fill both -expand 1
}

proc ag_stdout_init {} {
    global tcl_platform
    if { 0 && $tcl_platform(platform) == "windows" } {
	ag_stdout_init_window
	#ag_puts  "Stdout window initialized ..."
  }
}

proc ag_puts s {
    global tcl_platform
    if { 0 && $tcl_platform(platform) == "windows" } {
		.stdout.text insert end "$s\n"
		.stdout.text yview moveto 1
    } else {
		puts $s
    }
}

set source_callback_list ""

proc register_source_callback cbproc {
    global source_callback_list
    lappend source_callback_list $cbproc
}

proc unregister_source_callback cbproc {
    global source_callback_list
    set idx [lsearch -exact $source_callback_list $cbproc]
    set source_callback_list [lreplace $source_callback_list $idx $idx]
}

proc invoke_source_callback {cmd src} {
    global source_callback_list
    foreach c $source_callback_list {
        $c $cmd $src
    }
}

#
# Watch for new sources. If we get an activated video, go ahead and
# act like the user clicked on it.
#

proc ag_init_resources {} {
    option add Vic.ag_window_grid_width 3 startupFile
    option add Vic.ag_window_grid_height 3 startupFile
    option add Vic.ag_window_grid_mode small startupFile
    option add Vic.ag_window_grid_base_x 0 startupFile
    option add Vic.ag_window_grid_base_y 0 startupFile


    if [ file exists "vic.prefs" ] {
		#ag_puts "reading prefsfile  ..."
		option readfile "vic.prefs" startupFile
    } else {
		#ag_puts "no prefsfile available..."
    }
}

proc ag_init_window_grid {} {

    global ag_window_grid_width ag_window_grid_height
    global ag_window_cur_x ag_window_cur_y
    global ag_vwin ag_win ag_window_grid_mode
    global ag_window_grid_base_x ag_window_grid_base_y
    global ag_autoplace_active
    global ag_window_grid_placement

    set ag_vwin(small,ntsc,width) 160
    set ag_vwin(small,ntsc,height) 120
    set ag_vwin(small,cif,width) 176
    set ag_vwin(small,cif,height) 144

    set ag_vwin(medium,ntsc,width) 320
    set ag_vwin(medium,ntsc,height) 240
    set ag_vwin(medium,cif,width) 352
    set ag_vwin(medium,cif,height) 288

    set ag_vwin(large,width) 640
    set ag_vwin(large,ntsc,height) 480
    set ag_vwin(large,cif,width) 704
    set ag_vwin(large,cif,height) 576

    # platform-specific window sizes
    # - small
    set ag_win(small,x11,width) 184
    set ag_win(small,x11,height) 181

    set ag_win(small,aqua,width) 177
    set ag_win(small,aqua,height) 167

    set ag_win(small,win32,width) 184
    set ag_win(small,win32,height) 171

    # - medium sizes
    set ag_win(medium,x11,width) 360
    set ag_win(medium,x11,height) 347

    set ag_win(medium,aqua,width) 355
    set ag_win(medium,aqua,height) 313

    set ag_win(medium,win32,width) 360
    set ag_win(medium,win32,height) 315

    # - large sizes
    set ag_win(large,x11,width) 710
    set ag_win(large,x11,height) 606

    set ag_win(large,aqua,width) 705
    set ag_win(large,aqua,height) 601

    set ag_win(large,win32,width) 712
    set ag_win(large,win32,height) 603

    set ag_window_cur_x 0
    set ag_window_cur_y 0

    set ag_window_grid_width [option get . ag_window_grid_width Vic]
    set ag_window_grid_height [option get . ag_window_grid_height Vic]

    #ag_puts "Got grid width $ag_window_grid_width"

    set ag_window_grid_mode [option get . ag_window_grid_mode Vic]
    set ag_window_grid_placement [option get . ag_window_grid_placement Vic]

    set ag_window_grid_base_x [option get . ag_window_grid_base_x Vic]
    set ag_window_grid_base_y [option get . ag_window_grid_base_y Vic]

    if { "true" == [option get . disable_autoplace Vic] } {
	set ag_autoplace_active 0
    } else {
	set ag_autoplace_active [yesno ag_enable_window_grid]
    }

}

#
# Allocate a slot for this source based
# on the current policy
#
proc ag_alloc_slot {src} {
    return [ag_alloc_slot_adhoc $src 0 0]
#    return [ag_alloc_slot_by_site $src]
}

proc ag_gethostbyaddr addr {
    global tcl_platform

    if {$tcl_platform(platform) == "windows"} {
	return [gethostbyaddr $addr]
    } else {
	set fd [open "|dig +pfset=0x2020 -x $addr"]
	set l [gets $fd]
#	ag_puts "read $l"
	close $fd
	set d [split $l " "]
	set name [lindex $l [expr [llength $l] - 1]]
	
	return [string trimright $name .]
#	return [host_info official_name $addr]
    }
}

proc ag_alloc_slot_by_site {src} {
    set cname [$src sdes cname]
#    ag_puts "cname is $cname"
    set d [split $cname @]
    set addr [lindex $d 1]
#    ag_puts "addr is $addr"
    set name [ag_gethostbyaddr $addr]
#    ag_puts "name is $name"

    set slot [ag_find_site_slot $name]

    if {$slot == ""} {
	set slot [ag_alloc_slot_adhoc $src 5 0]
    }
    
    return $slot
}

proc ag_find_site_slot name {
    global ag_site_map
    while { $name != "" } {
	if [info exists ag_site_map($name) ] {
	    return [ag_alloc_site_slot $name]
	} else {
	    set dot [string first . $name]
	    if { $dot < 0 } {
		return ""
	    } else {
		set name [string range $name [expr $dot + 1] end]
	    }
	}
    }
}

proc ag_site_entry {site row} {
    global ag_site_map

    set ag_site_map($site) $row
    set ag_site_map($row,nextcol) 0
}

proc ag_init_site_map {} {
    global ag_window_cur_y

    ag_site_entry geode.mcs.anl.gov 0
    ag_site_entry jade.mcs.anl.gov 1
    ag_site_entry uky.edu 2
    ag_site_entry anl.gov 2
    ag_site_entry ncsa.uiuc.edu 3
    
#    set ag_window_cur_y 4
}

proc ag_alloc_site_slot name {
    global ag_site_map

    set y $ag_site_map($name)
    set x $ag_site_map($y,nextcol)
    incr ag_site_map($y,nextcol)

#    ag_puts "allocated $x $y for $name"

    return [list $x $y]
}

#
# Allocate a slot in a grid, filling rows & columns
#
#
proc ag_alloc_slot_adhoc {src xoffset yoffset} {
    global ag_window_grid_width ag_window_grid_height
    global ag_window_cur_x ag_window_cur_y
    global ag_window_grid_placement

    #ag_puts "Allocating slot: cur=$ag_window_cur_x $ag_window_cur_y size=$ag_window_grid_width $ag_window_grid_height"
    #ag_puts "Alignment is $ag_window_grid_placement"
    if {$ag_window_cur_x >= $ag_window_grid_width || $ag_window_cur_y >= $ag_window_grid_height} {
	#	ag_puts "No slots available"
		return ""
    } else {
		if {$ag_window_grid_placement == "horizontal"} {
			set x [expr $ag_window_cur_x + $xoffset]
			set y [expr $ag_window_cur_y + $yoffset]
			
			set ret [list $x $y]
			incr ag_window_cur_x
			if { $ag_window_cur_x >= $ag_window_grid_width } {
				set ag_window_cur_x 0
				incr ag_window_cur_y
			}
			return $ret
		} else {
			set x [expr $ag_window_cur_x + $xoffset]
			set y [expr $ag_window_cur_y + $yoffset]
			
			set ret [list $x $y]
			incr ag_window_cur_y
			if { $ag_window_cur_y >= $ag_window_grid_height } {
				set ag_window_cur_y 0
				incr ag_window_cur_x
			}
			return $ret
		}
    }
}

proc ag_map_slot {x y} {
    global ag_win ag_window_grid_mode
    global ag_window_grid_base_x ag_window_grid_base_y
    global tcl_platform
    
    set platform $tcl_platform(platform)
    set screen_x [expr $ag_window_grid_base_x + $x * $ag_win($ag_window_grid_mode,[ windowingsystem],width)]
    set screen_y [expr $ag_window_grid_base_y + $y * $ag_win($ag_window_grid_mode,[ windowingsystem],height)]
    
    #ag_puts "ag_window_grid_base x,y = $ag_window_grid_base_x , $ag_window_grid_base_y"
    #ag_puts "ag_map_slot returns $screen_x , $screen_y"
    return [list $screen_x $screen_y]
}


proc ag_set_initial_location {src x y } {
    global ag_cif_initial_width ag_cif_initial_height
    global ag_ntsc_initial_width ag_ntsc_initial_height
    global ag_vwin ag_win ag_window_grid_mode
    
    if [isCIF [rtp_format $src]] {
	set fmt cif
    } else {
	set fmt ntsc
    }

    set width $ag_vwin($ag_window_grid_mode,$fmt,width)
    set height $ag_vwin($ag_window_grid_mode,$fmt,height)

    ag_set_initial_location_and_size $src $x $y $width $height
}

proc ag_set_initial_location_and_size {src x y width height} {
    global userwin_size userwin_x userwin_y
    
    #ag_puts "new source $src has x=$x y=$y width=$width height=$height"
    
    set userwin_x($src) $x
    set userwin_y($src) $y
    set userwin_size($src) [join "$width x $height" ""]
}

proc ag_src_ident {src} {
    set n [$src sdes name]
    if { $n == "" } {
	set n [$src sdes cname]
    }
    return $n
}

#
# Re-apply autoplacement; invoked from the UI code
#
# Reset the current state of grid placement
# Close all the windows
# Scan the list of sources we've seen, and place them
#
proc ag_autoplace_apply {active} {
    global ag_slot_allocated ag_src_window
    global win_list
    global presenters_started
    global ag_autoplace_active
    global window_glue

    set ag_autoplace_active $active
    if {!$active} {return}

    set sources {}
    set glued_sources {}

    #
    # Build a list of pairs (name, src)
    # and sort on the name.
    #

    set src_name_list {}

    foreach src [array names presenters_started] {
	set n [ag_src_ident $src]
	if { $n != "" } {
	    lappend src_name_list [list $n $src]
	}
    }

    set sorted [lsort -dictionary -index 0 $src_name_list]
	
    foreach item $sorted {
	set src [lindex $item 1]
	set name [lindex $item 0]
	lappend sources $src

	#ag_puts "Found source $name $src"

	set glue_source($src) 0
	if [info exists ag_slot_allocated($src)] {

#	    ag_puts "Source had been auto-opened"

	    if [info exists ag_src_window($src) ] {
		set w $ag_src_window($src)
		if  { $window_glue($w) } {
		    set glue_source($src) 1
		    set glued_sources [lappend $glued_sources $src]
		} else {
		    unset ag_src_window($src)
		    catch " destroy_userwin $w "
		}
	    } else {
		# Somehow the window got opened and we didn't record the fact.
		# Figure out which one it likely is
		# 
		foreach ww win_list($src) {
		    if [viewing_window $ww] {
			puts "Destroying2 window $ww"
			catch "destroy_userwin $ww"
			puts "Done2 destroying window $ww"
		    }
		}
	    }	

	    if { !$glue_source($src) } {
		unset ag_slot_allocated($src)
	    }
	} else {
#	    ag_puts "Source not already opened"
	}
    }

    global ag_window_cur_x ag_window_cur_y

    set ag_window_cur_x 0
    set ag_window_cur_y 0

    foreach src $sources {
	#
	# See if this source is muted
	#

	if { [$src mute] != "0" } {
	    #ag_puts "$src is muted, skipping"
	    continue
	} elseif { $glue_source($src) == 1	 } {
	    #ag_puts "$src is glued, skipping"
	    continue
	} else {
	    #ag_puts "Placing $src [ag_src_ident $src]"
	}
	
	
#	ag_puts "Try to allocate slot for $src"
	ag_match_source $src

	if { $ag_slot_allocated($src) } {
#	    ag_puts "Got slot, openeing"
	    ag_open_source $src
	}
    }
}    	

#
# Open up a source. Snapshot the list of windows before and
# after the select_thumbnail so that we can keep track of
# what window got opened for it.
#
proc ag_open_source src {
    global win_list
    global ag_src_window

    set wl $win_list($src)
    select_thumbnail foo $src
    
#    puts "opening $src"
    foreach w $win_list($src) {
	if {[lsearch -exact $wl $w] == -1 } {
#	    puts "Aha, looks like we opened $w"
	    set ag_src_window($src) $w
	    set win $w
	    break  

	} else {
#	    puts "Found window $w from old list"
	}
    }

    if {![info exists win]} {
	#ag_puts "HMM, didn't find window for $src"
	return
    }

    #
    # Find the toplevel window
    #

    set top [winfo toplevel $win]
#    ag_puts "toplevel window is $top"
    bind $top <Configure> "ag_window_moved $src $win $top %W %T"
}

proc ag_window_moved {src win topwin eventwin type} {
    if { $topwin == $eventwin } {
#	ag_puts "AG window moved $src $win type=$type"
    }
}

#
# Entrypoint into slot allocation code
# Invoked when a source's CNAME shows up 
#
#
proc ag_match_source src {
    global ag_initial_info
    global userwin_x userwin_y userwin_size
    global ag_slot_allocated

    set ag_slot_allocated($src) 0
    set ssrc [$src ssrc]
#    ag_puts "new source $src $ssrc"

    #
    # Check if there's a preloaded position for this source
    #
    if [info exists ag_initial_info($ssrc)] {
	set info $ag_initial_info($ssrc)
	set x [lindex $info 4]
	set y [lindex $info 5]
	set width [lindex $info 2]
	set height [lindex $info 3]

	ag_set_initial_location_and_size $src $x $y $width $height
    } else {
	#
	# No saved config for this
	# Allocate a spot in the window grid
	#
	# Grid is defined by ag_window_grid_{width,height}
	# Currently available slot is ag_window_cur_{x,y}
	#
	set slot [ag_alloc_slot $src]
	if {$slot == "" } {
#	    ag_puts "No window slot available"
	} else {
	    set x [lindex $slot 0]
	    set y [lindex $slot 1]
#	    ag_puts "Got slot $x at $y"

	    set pos [ag_map_slot $x $y]
	    ag_set_initial_location $src [lindex $pos 0] [lindex $pos 1]
	    
	    set ag_slot_allocated($src) 1
	}
    }
}

proc ag_auto_activate {cmd src} {
    global ag_slot_allocated
    global presenters_started
    global ag_source_active
    global ag_autoplace_active
    global ag_src_window

    if { $cmd == "activate" } {
	set ag_source_active($src) 1
    } elseif { $cmd == "deactivate" } {
	set ag_source_active($src) 0
    } elseif { $cmd == "register" } {
    } elseif { $cmd == "unregister" } {
	#
	# Unregister comes when a stream goes away
	#
	set ret [catch {
	    unset ag_slot_allocated($src)
	    unset presenters_started($src)
	    unset ag_src_window($src)
	} reterror]
	if { $ret != 0 } {
	    #puts "Unregister caught error $reterror"
	    catch {
		#puts "Source was [$src sdes cname]"
	    }
	}
    } elseif { $cmd == "sdes_update" } {

	if { [$src sdes cname] != "" || 1 } {

	    if {! [info exists presenters_started($src) ] } {

		if [ag_source_active $src] {

		    #ag_puts "New presenter $src"
		    set presenters_started($src) 1

		    if {!$ag_autoplace_active} {
			return
		    }

		    ag_match_source $src

		    if { $ag_slot_allocated($src) } {
			ag_open_source $src
		    }
		}
	    }
	}
    }
}

proc ag_source_active src {
    global ag_source_active
    if {[info exists ag_source_active($src)] && $ag_source_active($src) } {
	return 1
    } else {
	return 0;
    }	
}

proc enum_windows {} {

	toplevel .apholder
	#pack .apholder

	set mon [get_monitors]
	#ag_puts "have monitors $mon"
	ag_autoplace::ag_build_autoplacer .apholder $mon
}


proc enum_windows_foo {} {
    global win_src

#    set fp [ open {c:/temp/windows.txt} w ]
    set fp stdout
    set kids [winfo children .]
    foreach k $kids {
	if [viewing_window $k] {
	    puts $fp "$k is a vid window"
	    set v $k.frame.video
	    set src $win_src($v)
	    set ssrc [$src ssrc]
	    set cname [$src sdes cname]
	    set width [winfo width $k]
	    set height [winfo height $k]
	    set x [winfo x $k]
	    set y [winfo y $k]
	    puts $fp "$ssrc {$cname} $width $height $x $y"
	}
    }

#    close $fp
}

proc ag_load_config config_file {
    global ag_initial_info
    set fp [open $config_file]
#    debug_show "opened $config_file"
    while {[gets $fp line] != -1} {
	set ssrc [lindex $line 0]
	set ag_initial_info($ssrc) $line
    }
    close $fp
}

proc init_ag {} {

    ag_stdout_init
    ag_init_resources
    ag_init_window_grid
    ag_init_site_map
    catch {console hide}

    #ag_puts "enable=[option get . ag_enable_window_grid Vic]";
#    if [yesno ag_enable_window_grid] {
	register_source_callback ag_auto_activate
#    }

    # figure out the monitor list. If we're not ddraw/multimon, fake it.
    #
    set mon_list [get_monitors]
    if { $mon_list == ""} {
	set top .top
	#ag_puts "Did not find monitor list"
	set mon_list [list [ list 0 0 [winfo screenwidth $top] [winfo screenheight $top] "main" 0]]
    }
    #ag_puts "Using monitor list $mon_list"

    init_pixrate $mon_list
}

if {$tcl_platform(platform) != "windows"} {
    init_ag
}
