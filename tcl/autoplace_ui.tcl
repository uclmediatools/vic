# changes, Roman: 
#	- indentations included ;)
#
#	- update the grid-size after pressing the radiobuttons (small,medium,large)
#
#	- insert windows placement: horizontally or vertically
#      
#       - new feature: press shif+mousebutton1 to drag the whole placinggrid
#
#	- new visually subdivisioning of the placinggrid
#
#       - new arrangement of ui-frames, I hope this was OK.
#
#	
#	to be done: do not place videowindows "between" displays


namespace eval ag_autoplace {
    namespace export show_ui

    variable therect
    variable scale
    variable startx
    variable starty
    variable endx
    variable endy

    variable maxx
    variable minx
    variable maxy
    variable miny
    variable canvas

    variable ap_active

    variable top

    proc show_ui {} {
		variable top
		variable ap_active
		global ag_autoplace_active

		if [info exists ag_autoplace_active] {
			set ap_active $ag_autoplace_active
		} else {
			set ap_active 0
		}

		if { ! [info exists top ]} {

			#ag_puts "in show_ui [namespace current]"
			set top [build_ui]
		}

		init_ui

		wm deiconify $top 
    }

    proc init_ui {} {
		global tcl_platform ag_win ag_window_grid_mode
		global ag_window_grid_base_x ag_window_grid_base_y
		global ag_window_grid_width ag_window_grid_height
		
		variable therect
		variable scale
		variable startx
		variable starty
		variable endx
		variable endy
		variable maxx
		variable minx
		variable maxy
		variable miny
		variable ap_active
		variable canvas
	    
		set platform $tcl_platform(platform)

		set win_width $ag_win($ag_window_grid_mode,[windowingsystem],width)
		set win_height $ag_win($ag_window_grid_mode,[windowingsystem],height)

		if { $ap_active } { 
			set startx [expr int($ag_window_grid_base_x * $scale)]
			set starty [expr int($ag_window_grid_base_y * $scale)]
			set endx [expr int(($ag_window_grid_base_x + $ag_window_grid_width * $win_width)*$scale)]
			set endy [expr int(($ag_window_grid_base_y + $ag_window_grid_height * $win_height)*$scale)]

			update_grid_vars $canvas

			if [ info exists therect ] {
			$canvas delete $therect
			}
			# set therect [$canvas create rectangle $startx $starty $endx $endy -fill blue]
		}
    }

    proc build_ui {} {

		variable top
		variable ap_active

		set top .autoplace
		toplevel $top

		set mon_list [get_monitors]
		if { $mon_list == ""} {
			#ag_puts "Did not find monitor list"
			set mon_list [list [ list 0 0 [winfo screenwidth $top] [winfo screenheight $top]]]
		}
		#ag_puts "Using monitor list $mon_list"
		frame $top.ap -relief ridge -borderwidth 3
		pack $top.ap -fill both -expand 1 
		ag_build_autoplacer $top.ap $mon_list


		#
		# statusframe below the windowscanvases
		#
		frame $top.status -relief flat
		pack  $top.status -fill both
		
		global ag_window_grid_placement
		

		#
		# alignment radiobuttons
		#
		frame       $top.status.direction     -relief ridge -borderwidth 3
		label       $top.status.direction.label           -text "Arrangement direction"
		frame       $top.status.direction.arr -relief flat
		radiobutton $top.status.direction.arr.horizontal  -text "Horizontal" -variable ::ag_window_grid_placement -value "horizontal"
		radiobutton $top.status.direction.arr.vertical    -text "Vertical"   -variable ::ag_window_grid_placement -value "vertical"  
		pack        $top.status.direction.label          -side top
		pack        $top.status.direction.arr            -side top
		pack        $top.status.direction.arr.horizontal -side left -anchor w
		pack        $top.status.direction.arr.vertical   -side left -anchor w
		pack        $top.status.direction            -side left -fill x -expand 1
		
		if {$ag_window_grid_placement != "horizontal" && $ag_window_grid_placement != "vertical" } {
			#ag_puts "ag_window_placement non existent!"
			set ag_window_grid_placement "horizontal"
		}
		
		#
		# videosize radiobuttons
		#
		global ag_window_grid_mode
		if {! [info exists ag_window_grid_mode] } { 
			set ag_autoplace_size medium 
		}

		frame       $top.status.sizef -relief ridge -borderwidth 3
		label       $top.status.sizef.label           -text "Video size"
		frame       $top.status.sizef.arr -relief flat
		radiobutton $top.status.sizef.arr.small  -text "Small"  -variable ::ag_window_grid_mode -value small  -command [namespace code {update_grid_vars x 1}]
		radiobutton $top.status.sizef.arr.medium -text "Medium" -variable ::ag_window_grid_mode -value medium -command [namespace code {update_grid_vars x 1}]
		radiobutton $top.status.sizef.arr.large  -text "Large"  -variable ::ag_window_grid_mode -value large  -command [namespace code {update_grid_vars x 1}]
		pack        $top.status.sizef.label          -side top 
		pack        $top.status.sizef.arr            -side top
		pack        $top.status.sizef.arr.small  -side left -anchor w
		pack        $top.status.sizef.arr.medium -side left -anchor w
		pack        $top.status.sizef.arr.large  -side left -anchor w
		pack        $top.status.sizef        -side left -fill x -expand 1
		

		#
		# activate autoplace
		#
		checkbutton $top.status.active -text "Autoplace activated" -variable ag_autoplace::ap_active 
		pack        $top.status.active -side right
		
		
		#
		# create a bottomframe 
		#
		frame  $top.bottomframe -relief flat
		
		
		#
		# Gridinfo labels
		#
		frame $top.bottomframe.textbox -relief flat
		label $top.bottomframe.textbox.gridw -text "Grid width: 0"
		label $top.bottomframe.textbox.gridh -text "Grid height: 0"
		pack  $top.bottomframe.textbox.gridw -side left
		pack  $top.bottomframe.textbox.gridh -side left
		pack  $top.bottomframe.textbox       -side left
		
		#
		# buttons: Apply, Write Prefs, Close
		#
		frame  $top.bottomframe.controls -relief flat
		button $top.bottomframe.controls.apply -text "Apply"            -command [namespace code {apply_placement}]
		button $top.bottomframe.controls.write -text "Write vic.prefs"  -command [namespace code "write_prefs"]
		button $top.bottomframe.controls.quit  -text "Close"            -command "[namespace code {wm withdraw}] $top"
		pack   $top.bottomframe.controls.apply -side left
		pack   $top.bottomframe.controls.write -side left
		pack   $top.bottomframe.controls.quit  -side left
		pack   $top.bottomframe.controls       -side right
		pack $top.bottomframe -side bottom -fill both

		# added to catch window close action
		wm protocol $top WM_DELETE_WINDOW "wm withdraw $top"

		return $top
    }

    proc activate_toggle {} {
		variable ap_active

		puts "toggle, ap_active is $ap_active"

		variable top

		if { $ap_active } {
			$top.controls.apply configure -state normal
		} else {
			$top.controls.apply configure -state disabled
		}
    }


    proc apply_placement {} {

		global tcl_platform ag_win ag_window_grid_mode
		global ag_window_grid_base_x ag_window_grid_base_y
			global ag_window_grid_width ag_window_grid_height

		variable grid_width
		variable grid_height

		variable grid_top_x
		variable grid_top_y

		if ![ info exists grid_top_x ] { return } 

		set ag_window_grid_base_x $grid_top_x
		set ag_window_grid_base_y $grid_top_y

		set ag_window_grid_width $grid_width
		set ag_window_grid_height $grid_height

		variable ap_active
		ag_autoplace_apply $ap_active
    }
    
    proc write_prefs {} {
		#ag_puts "write_prefs !"
		
		variable parcel_width
		variable parcel_height
		variable grid_width
		variable grid_height

		variable grid_top_x
		variable grid_top_y
		variable grid_bot_x
		variable grid_bot_y
		variable ap_active

		global ag_window_grid_mode
		global ag_window_grid_placement

		if ![ info exists grid_top_x ] { return } 

		if {$ag_window_grid_placement != "horizontal" && $ag_window_grid_placement != "vertical"} {
			set ag_window_grid_placement "horizontal"
		}


		if { $ap_active} { set a "true" } else { set a "false" }

		set fh [open "vic.prefs" "w"]
		
		puts $fh "*ag_window_grid_mode: $ag_window_grid_mode"
		puts $fh "*ag_enable_window_grid: $a"
		puts $fh "*ag_window_grid_width: $grid_width"
		puts $fh "*ag_window_grid_height: $grid_height"
		puts $fh "*ag_window_grid_base_x: $grid_top_x"
		puts $fh "*ag_window_grid_base_y: $grid_top_y"
		#
		# store the placementdirection
		#
		puts $fh "*ag_window_grid_placement: $ag_window_grid_placement"
		close $fh
    }

    proc ag_build_autoplacer {w mon_list} {

		variable therect
		variable canvas
		variable scale
		variable startx
		variable starty
		variable endx
		variable endy
		variable maxx
		variable minx
		variable maxy
		variable miny
		variable canv_width
		variable canv_height

		#
		# mon_list is a list of rects that define the
		# monitors used in the syestem.
		#
		# Compute an aggregate top-left and bottom-right rect, in monitor
		# coordinate space and use that as the size of this canvas.
		#

		
		foreach mon $mon_list {
			set montopx [lindex $mon 0]
			set montopy [lindex $mon 1]
			set monbotx [lindex $mon 2]
			set monboty [lindex $mon 3]
			if [ info exists minx ] {
			if { $montopx < $minx } { set minx $montopx }
			if { $montopy < $miny } { set miny $montopy }
			if { $monbotx > $maxx } { set maxx $monbotx }
			if { $monboty > $maxy } { set maxy $monboty }
			} else {
			set minx $montopx
			set miny $montopy
			set maxx $monbotx
			set maxy $monboty
			}
		}

		set mon_width [ expr $maxx - $minx + 1]
		set mon_height [ expr $maxy - $miny + 1]


		#
		# compute a scale to bring the max width to half the current screen,
		# and max height to half the current screen height.
		#

		set xtarg [expr [winfo screenwidth $w] / 2.0]
		set xscale [expr $xtarg / $mon_width]

		set ytarg [expr [winfo screenheight $w] / 2.0]
		set yscale [expr $ytarg / $mon_height]

		# ag_puts "$xtarg $xscale  $ytarg $yscale $mon_width $mon_height"

		if { $xscale < $yscale } {
			set scale $xscale
		} else {
			set scale $yscale
		}

		if { $scale > 1 } {
			set scale 1
		}
		
		# Adjust max/min to canvas coords for later use in clipping
		#
		set maxx [ expr round($maxx * $scale) ]
		set maxy [ expr round($maxy * $scale) ]
		set minx [ expr round($minx * $scale) ]
		set miny [ expr round($miny * $scale) ]

		set canv_width [expr $scale * $mon_width + 1]
		set canv_height [expr $scale * $mon_height + 1]

		puts "xscale=$xscale yscale=$yscale using=$scale"

		set c $w.canvas
		# set the global canvas
		set canvas $c  
		canvas $c -width $canv_width  -height $canv_height

		pack $c -anchor center -expand 1

		$c xview moveto 0
		$c yview moveto 0

	#	bind $c <Configure> {
	#	    %W xview moveto 0
	#	    %W yview moveto 0
	#	}

		#
		# Draw the monitor rects
		#

		foreach mon $mon_list {
			set montopx [expr round([lindex $mon 0] * $scale - $minx)]
			set montopy [expr round([lindex $mon 1] * $scale - $miny)]
			set monbotx [expr round([lindex $mon 2] * $scale - $minx)]
			set monboty [expr round([lindex $mon 3] * $scale - $miny)]

			puts "$montopx $montopy $monbotx $monboty"

			set r [$c create rectangle $montopx $montopy $monbotx $monboty  -outline red  -fill white]
		}

		
		bind $c <ButtonPress-1> "[namespace code start_rubberband] $c %x %y"
		bind $c <B1-Motion> "[namespace code continue_rubberband] $c %x %y"
		bind $c <ButtonRelease-1> "[namespace code finish_rubberband] $c "
		
		# to move the displaygrid while pressing shift-button1
		bind $c <Shift-ButtonPress-1> "[namespace code start_move] $c %x %y"
		bind $c <Shift-B1-Motion> "[namespace code continue_move] $c %x %y"
		
    }


    proc update_grid_vars {{lcanvas ""} {use_global_canvas 0}} {
		variable scale
		variable startx
		variable starty
		variable endx
		variable endy
		variable maxx
		variable minx
		variable maxy
		variable miny

		variable parcel_width
		variable parcel_height
		variable grid_width
		variable grid_height

		variable grid_top_x
		variable grid_top_y
		variable grid_bot_x
		variable grid_bot_y
		variable win_width
		variable win_height
		
		variable rect_list
		variable rect_x
		variable rect_y
		
		variable canvas
		variable therect
		
		#
		# Calculate the grid sizes
		#
		global tcl_platform ag_win ag_window_grid_mode
		global ag_window_grid_base_x ag_window_grid_base_y
		global ag_window_grid_width ag_window_grid_height

		set platform $tcl_platform(platform)
		
		set win_width $ag_win($ag_window_grid_mode,[windowingsystem],width)
		set win_height $ag_win($ag_window_grid_mode,[windowingsystem],height)

		#
		# Sort the start/end positions so that start is at top left
		# and end is at bottom right
		#

		if { $startx > $endx } { 
			set grid_top_x $endx
			set grid_bot_x $startx
			set rect_x $endx
		} else {
			set grid_top_x $startx
			set grid_bot_x $endx
			set rect_x $startx
		}
		if { $starty > $endy } { 
			set grid_top_y $endy
			set grid_bot_y $starty
			set rect_y $endy
		} else {
			set grid_top_y $starty
			set grid_bot_y $endy
			set rect_y $starty
		}
		
		#
		# Translate and scale to screen coordinates, 
		#

		set grid_top_x [expr int(($grid_top_x + $minx) / $scale)]
		set grid_top_y [expr int(($grid_top_y + $miny) / $scale)]
		set grid_bot_x [expr int(($grid_bot_x + $minx) / $scale)]
		set grid_bot_y [expr int(($grid_bot_y + $miny) / $scale)]

		set parcel_width [expr $grid_bot_x - $grid_top_x]
		set parcel_height [expr $grid_bot_y - $grid_top_y]

		set grid_width [expr int(floor($parcel_width / $win_width))]
		set grid_height [expr int(floor($parcel_height / $win_height))]
	
		# the next three lines came from continue_rubberband
		variable top
		$top.bottomframe.textbox.gridw configure -text "Grid width: $grid_width"
		$top.bottomframe.textbox.gridh configure -text "Grid height: $grid_height"

		if {$use_global_canvas == 1} {
			set lcanvas $canvas
		}

		if {$lcanvas != "" }  {
			#
			# delete the whole rectanglelist to display new rects
			#
			if [info exists rect_list] {
				foreach rect $rect_list {
					$lcanvas delete $rect
				}
			}
			# ag_puts "Grid Top x: $grid_top_x"
			
			#
			# draw the new rectangles and append them to a list, so that we can destroy them later
			#
			# ag_puts "Redraw rectangles in rectangle width x height: $grid_width x $grid_height"
			for {set i 0} {$i < $grid_width} {incr i 1} {
				for {set ii 0} {$ii < $grid_height} {incr ii 1} {
					lappend rect_list [$lcanvas create rectangle [expr $rect_x + $i * $win_width* $scale] [expr $rect_y + $ii * $win_height* $scale] [expr $rect_x + ($i+1) * $win_width* $scale] [expr $rect_y + ($ii+1) * $win_height* $scale] -outline black -fill blue -width 1]
					#ag_puts "$lcanvas create rectangle [expr $startx + $i * $win_width* $scale] \
					#						[expr $starty + $ii * $win_height* $scale] \
					#						[expr $startx + ($i+1) * $win_width* $scale] \
					#						[expr $starty + ($ii+1) * $win_height* $scale] \
					#						-outline black -fill blue -width 1"
				
				}
			}
			$lcanvas coords $therect $startx $starty $endx $endy
		}

				
		#ag_puts ""
		#ag_puts "grid_top_x: $grid_top_x"
		#ag_puts "grid_top_y: $grid_top_y"
		#ag_puts "grid_bot_x: $grid_bot_x"
		#ag_puts "grid_bot_y: $grid_bot_y"
		#ag_puts "parcel_width: $parcel_width"
		#ag_puts "parcel_height: $parcel_height"
		#ag_puts "grid_width: $grid_width"
		#ag_puts "grid_height: $grid_height"
		#ag_puts "win_width: $win_width"
		#ag_puts "win_heigth: $win_height"
    }

	variable poi_x
	variable poi_y
	
	proc start_move {c x y} {
		# move the windowsplacinggrid by pressing shift and the first mouse button
		variable poi_x
		variable poi_y
		
		set poi_x $x
		set poi_y $y
	}
	
	proc continue_move {c x y} {
		# move the windowsplacinggrid by pressing shift and the first mouse button
		variable poi_x
		variable poi_y
		
		variable delta_x
		variable delta_y
		
		variable startx
		variable starty
		variable endx
		variable endy
		
		set delta_x [expr $x - $poi_x]
		set delta_y [expr $y - $poi_y]
		
		set startx [expr $startx + $delta_x]
		set starty [expr $starty + $delta_y]
		
		set endx [expr $endx + $delta_x]
		set endy [expr $endy + $delta_y]

		set poi_x $x
		set poi_y $y
		
		# ag_puts "Start Move: $tmp_x $tmp_y"
		update_grid_vars x 1
	}
	
	
	
	
    proc start_rubberband {c x y} {
		variable therect
		variable gridrect
		variable scale
		variable startx
		variable starty
		variable endx
		variable endy
		variable maxx
		variable minx
		variable maxy
		variable miny
		variable canv_width
		variable canv_height


		if { $x < 0 } { set x 0 }
		if { $x > $canv_width } { set x $canv_width }
		if { $y < 0 } { set y 0 }
		if { $y > $canv_height } { set y $canv_height; puts "max" }

		set startx $x
		set starty $y
		set endx $x
		set endy $y

		if [ info exists therect ] {
			$c delete $therect
		}

		if [info exists gridrect ] {
			$c delete $gridrect
		}
		
		set therect [$c create rectangle $x $y $x $y]
		update_grid_vars

		variable grid_top_x
		variable grid_top_y
		variable grid_width
		variable grid_height
		variable win_width
		variable win_height
		variable minx
		variable miny

		set gr_x [expr int($grid_top_x * $scale - $minx)]
		set gr_y [expr int($grid_top_y * $scale - $miny) ]
		set gr_xe [expr int(($grid_top_x + $win_width * $grid_width) * $scale)]
		set gr_ye [expr int(($grid_top_y + $win_height * $grid_height) * $scale)]
    }	

    proc continue_rubberband {c x y} {
		variable therect
		variable gridrect
		variable scale
		variable startx
		variable starty
		variable endx
		variable endy
		variable maxx
		variable minx
		variable maxy
		variable miny
		variable top
		variable canv_width
		variable canv_height


		if { $x < 0 } { set x 0 }
		if { $x > $canv_width } { set x $canv_width }
		if { $y < 0 } { set y 0 }
		if { $y > $canv_height } { set y $canv_height; puts "max" }


		set endx $x
		set endy $y
		
		# the following line is moved to update_vars
		# $c coords $therect $startx $starty $endx $endy

		update_grid_vars $c
		variable grid_width 
		variable grid_height
		variable grid_top_x
		variable grid_top_y
		variable win_width
		variable win_height
		variable minx
		variable miny

		set gr_x [expr int($grid_top_x * $scale - $minx)]
		set gr_y [expr int($grid_top_y * $scale - $miny) ]
		set gr_xe [expr int(($grid_top_x + $win_width * $grid_width) * $scale)]
		set gr_ye [expr int(($grid_top_y + $win_height * $grid_height) * $scale)]

	#	puts "S: $startx $starty $endx $endy"
	#	puts "G: $scale $minx $miny $grid_top_x $grid_top_y $gr_x $gr_y $gr_xe $gr_ye"
	#	$c coords $gridrect $gr_x $gr_y $gr_xe $gr_ye

	#	moved to update_grid_vars
	#	$top.bottomframe.textbox.gridw configure -text "Grid width: $grid_width"
	#	$top.bottomframe.textbox.gridh configure -text "Grid height: $grid_height"
	}
	
	proc finish_rubberband {c } {
		# this proc was left intentionally empty
    }

}

#  global ag_win
#      set ag_win(small,unix,width) 250
#      set ag_win(small,unix,height) 200

#      set ag_win(small,windows,width) 185
#      set ag_win(small,windows,height) 172

#      set ag_win(medium,unix,width) 365
#      set ag_win(medium,unix,height) 338

#      set ag_win(medium,windows,width) 360
#      set ag_win(medium,windows,height) 322


#button .doit -text "show autoplacer" -command {ag_autoplace::show_ui}
#pack .doit

#  global ag_autoplace_size ag_autoplace_active

#  set ag_autoplace_size medium
#  set ag_autoplace_active 1
#  ag_autoplace::show_ui

#ag_autoplace::ag_build_autoplacer .apholder { { 0 0 1280 996 } { 1280 0 2304 768 } }

#{{0 0 1023 767} {1024 0 2047 767} {0 768 1279 1791} }

#
