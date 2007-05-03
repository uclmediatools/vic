
proc init_pixrate {monitors} {
    global pixrate pixrateMonitors 

    set gain 1
    rate_variable pixrate(total) $gain
    set pixrate(total) 0

    foreach mon $monitors {
	set idx [lindex $mon 5]
    	#puts "have monitor 'idx=$idx' '$mon'"
	rate_variable pixrate($idx) $gain
	set pixrate($idx) 0
	set pixrateMonitors($idx) [lindex $mon 4]
    }
}

proc create_pixrate_stats_window {} {

	set w .pixrate
	if [winfo exists $w] {
		stat_destroy $w
		return
	}  

	create_toplevel $w "Pixrate Stats"

	set f [smallfont]

	frame $w.title -borderwidth 2 -relief groove
	label $w.title.main -borderwidth 0 -anchor w \
		-text "Pixrate Statistics"
	frame $w.frame -borderwidth 2 -relief groove

	global stat_method win_src
	set stat_method($w) "update_pixrates"
	# hack
	set win_src($w) pixrate
	create_stats_panel $w.frame [update_pixrates]

	pack $w.title.main -anchor w
	pack $w.title -fill x
	pack $w.frame -expand 1 -fill x -anchor center

	wm geometry $w +[winfo pointerx .]+[winfo pointery .]
	wm deiconify $w
	# start up the timer
	window_timer $w stat_update

	button $w.dismiss -relief raised -font $f \
		-command "stat_destroy $w" -text Dismiss
	pack $w.dismiss -anchor c -pady 4
}

proc update_pixrates {} {
    global pixrate pixrateMonitors
	puts "in periodic_update"
	update_pixrates_vars
	puts "after update pixrates"
    set rate $pixrate(total)
    set r "total $rate"
    set r "$r total_bytes [expr $rate * 3]"

    foreach idx [array names pixrateMonitors] {
	set rate $pixrate($idx)
	set r "$r pixels_$idx $rate"
	set r "$r bytes_$idx [expr $rate * 3]"
    }
    puts "returning $r"
    return $r
}

proc pixrate {foo} {
    return [update_pixrates]
}
