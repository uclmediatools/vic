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

proc fork_histtolut { } {
	global V
	if { $V(dither) == "gray" } {
		open_dialog "cannot optimize grayscale rendering"
		return
	}
	#
	# create a histogram object and collect stats from decoders
	#
	set ch [$V(colorModel) create-hist]
	set active 0
	foreach src [session active] {
		set d [$src handler]
		if { ![$src mute] && $d != "" } {
			$d histogram $ch
			set active 1
		}
	}
	if !$active {
		open_dialog "no active, unmuted sources"
		delete $ch
		return
	}
	set pid [pid]
	set outfile /tmp/vicLUT.$pid
	set infile /tmp/vicHIST.$pid

	if { [$ch dump $infile] < 0 } {
		open_dialog "couldn't create $infile"
		delete $ch
		return
	}
	delete $ch
	#
	# fork off a histtolut and use tk fileevent to call back
	# finish_histtolut when the child is done
	#
	#XXX -n
	set eflag ""
	if { $V(dither) == "ed" } {
		set eflag "-e"
	}
	if [catch \
	  "open \"|histtolut $eflag -n 170 -o $outfile $infile\"" pipe] {
		open_dialog "histtolut not installed in your path"
		return
	}
	fileevent $pipe readable "finish_histtolut $pipe $infile $outfile"
	# disable button while histtolut is running
	$V(optionsMenu) entryconfigure "Optimize Colormap" \
		-state disabled
	.menu configure -cursor watch
}

proc finish_histtolut { pipe infile outfile } {
	.menu configure -cursor ""
	global V
	$V(optionsMenu) entryconfigure "Optimize Colormap" \
		-state normal
	set cm $V(colorModel)
	$cm free-colors
	$cm lut $outfile
	if ![$cm alloc-colors] {
		#XXX unset lut
		revert_to_gray
	}
	#XXX make this a proc
	foreach src [session active] {
		set d [$src handler]
		if { $d != "" } {
			$d redraw
		}
	}
	fileevent $pipe readable ""
	close $pipe
}

proc have_transmit_permission {} {
	global inputDeviceList
	return [expr [info exists inputDeviceList] && ![yesno recvOnly]]
}

#
# Build the menu panel.  Called from toggle_window,
# the first time the Menu button is hit.
#
proc build.menu { } {
	set w .menu
	create_toplevel $w "vic menu"
	wm withdraw $w
	catch "wm resizable $w false false"

	frame $w.session
	frame $w.cb
	build.xmit $w.cb
	frame $w.encoder
	build.encoder $w.encoder
	frame $w.decoder
	build.decoder $w.decoder
	global V
	set net $V(data-net)
	build.session $w.session [$net addr] [$net port] \
		[[srctab local] srcid] [$net ttl] [[srctab local] sdes name]

	button $w.dismiss -text Dismiss -borderwidth 2 -width 8 \
		-relief raised -anchor c \
		-command "toggle_window $w" -font [mediumfont]

	pack $w.cb $w.encoder $w.decoder $w.session -padx 6 -fill x
	pack $w.dismiss -anchor c -pady 4

	if [have_transmit_permission] {
		selectInitialDevice
	}
}

proc defaultDevice {} {
	set d [resource defaultDevice]
	if { $d == "" } {
		set d [resource device]
	}
	return $d
}	

proc selectInitialDevice {} {
	global videoDevice inputDeviceList
	set d [defaultDevice]
	foreach v $inputDeviceList {
		if { [$v nickname] == "$d" && \
			[$v attributes] != "disabled" } {
			set videoDevice $v
			select_device $v
			return
		}
	}
	foreach v $inputDeviceList {
		if { "[$v attributes]" != "disabled" &&
			"[$v nickname]" != "still" } {
			set videoDevice $v
			select_device $v
			return
                }
	}
}

proc build.session { w dst port srcid ttl name } {
	set f [smallfont]	

	label $w.title -text Session
	pack $w.title -fill x

	frame $w.nb -relief sunken -borderwidth 2
	pack $w.nb -fill x

	frame $w.nb.frame
	pack append $w.nb \
		$w.nb.frame { top fillx }

	label $w.nb.frame.info -font $f -anchor w \
		-text "Dest: $dst   Port: $port  ID: $srcid  TTL: $ttl"

	frame $w.nb.frame.name
	label $w.nb.frame.name.label -text "Name: " -font $f -anchor e -width 6
	mk.entry $w.nb.frame.name updateName $name
	pack $w.nb.frame.name.label -side left
	pack $w.nb.frame.name.entry -side left -expand 1 -fill x -pady 2

	frame $w.nb.frame.msg
	label $w.nb.frame.msg.label -text "Note: " -font $f -anchor e -width 6
	mk.entry $w.nb.frame.msg update_note ""
	pack $w.nb.frame.msg.label -side left
	pack $w.nb.frame.msg.entry -side left -expand 1 -fill x -pady 2

	mk.key $w.nb.frame

	frame $w.nb.frame.b

	button $w.nb.frame.b.stats -text "Global Stats" -borderwidth 2 \
		-anchor c -font $f -command create_global_window
	button $w.nb.frame.b.members -text Members -borderwidth 2 \
		-anchor c -font $f -command "toggle_window .srclist"

	pack $w.nb.frame.b.stats $w.nb.frame.b.members \
		-side left -padx 4 -pady 2 -anchor c

	pack $w.nb.frame.info $w.nb.frame.name $w.nb.frame.msg \
		$w.nb.frame.key \
		-fill x -padx 2 -expand 1
	pack $w.nb.frame.b -pady 2 -anchor c
}

proc setFillRate { } {
	global sendingSlides transmitButtonState V
	if { $transmitButtonState } {
		if { $sendingSlides } {
			$V(grabber) fillrate 16
		} else {
			$V(grabber) fillrate 2
		}
	}
}

proc updateName { w name } {
	set name [string trim $name]
	if { $name != ""} {
		[srctab local] sdes name $name
		return 0
	}
	return -1
}

proc update_note { w s } {
	set s [string trim $s]
	[srctab local] sdes note $s
	return 0
}

proc encoder args {
	global V
	if ![info exists V(encoder)] {
		puts "vic: encoder: no encoder"
		return
	}
	eval $V(encoder) $args
}

proc grabber args {
	global V
	if ![info exists V(grabber)] {
		# ignore
		return
	}
	eval $V(grabber) $args
}

proc create_encoder fmt {
	if { $fmt == "nvdct" } {
		set encoder [new module nv]
		$encoder use-dct 1
	} else {
		set encoder [new module $fmt]
	}
	return $encoder
}

set transmitButtonState 0
proc transmit { } {
	global transmitButtonState videoFormat videoDevice V useJPEGforH261 useHardwareComp
	if ![have grabber] {
		set DA [$videoDevice attributes]
		set DF [attribute_class $DA format]
		set DS [attribute_class $DA size]

		# first, see if device can produce bitstream directly
		# if not, try a transcoder
		# if that doesn't work, try anything else

		if { [inList $videoFormat $DF] } {
			if { $videoFormat == "h261" || $videoFormat == "cellb" || $videoFormat == "jpeg"} {
				# use special hardware tag...
				set encoder ""
				if  { $useHardwareComp } {
					set encoder [create_encoder "$videoFormat/hw"]
				}
				if { $encoder == "" } {
					set encoder [create_encoder "$videoFormat"]
				}
			} else {
				set encoder [create_encoder $videoFormat]
			}
			set grabtarget $encoder
			set grabq ""
		} elseif { $videoFormat == "h261" && [inList jpeg $DF] && \
				$useJPEGforH261 } {
			# just jpeg->h261 at this time
			set transcoder [new transcoder jpeg/dct]
			set encoder [new module h261/dct]
			$transcoder target $encoder
			set grabtarget $transcoder
			set grabq "70"
		} elseif { [inList $videoFormat [device_formats $videoDevice] ] } {
			set encoder [create_encoder $videoFormat]
			set grabtarget $encoder
			set grabq ""
		}

		$encoder transmitter $V(session)
		set V(encoder) $encoder
		set ff [$grabtarget frame-format]
		set V(grabber) [$videoDevice open $ff]
		# special cases
		if { $V(grabber) == "" && $ff == "411" } {
			# try cif instead of 411
			set V(grabber) [$videoDevice open cif]
		}
		if { $V(grabber) == "" } {
			#XXX
			puts "Error: grabber=\"\" puts XXX couldn't set up grabber/encoder for $ff->$videoFormat"
			exit 1
		}
		set error [$V(grabber) status]
		if { $error < 0 } {
			close_device
			if { $error == -2 } {
				#XXX hack: this happens when we tried to open
				# a jvideo with a non-jpeg format
				set transmitButtonState 0
				open_dialog \
				    "Can't use jvideo with $videoFormat format"
				select_device $videoDevice
				return
			}
			set transmitButtonState 0
			open_dialog \
			    "can't open [$videoDevice nickname] capture device"
			return
		}
		if ![tm_init $V(grabber) $grabtarget] {
			$V(grabber) target $grabtarget
		}
		init_grabber $V(grabber)
		if { $grabq != "" } {
			$V(grabber) q $grabq
		}
	}
	if [have capwin] {
		set w [winfo toplevel $V(capwin)]
		if $transmitButtonState {
			wm deiconify $w
		} else {
			wm withdraw $w
		}
		update idletasks
	}
	$V(grabber) send $transmitButtonState
}

proc close_device {} {
	global V
	delete $V(encoder)
	delete $V(grabber)
	unset V(grabber) V(encoder)
	if [info exists V(capwin)] {
		# delete the C++ object, then destrory the tk window
		delete $V(capwin)
		destroy [winfo toplevel $V(capwin)]
		unset V(capwin)
	}
}

proc release_device { } {
	global transmitButtonState transmitButton
	if [have grabber] {
		if $transmitButtonState {
			$transmitButton invoke
		}
		close_device
	}
}

proc configWinGrabber {} {
	global configOnTransmit
	grabber useconfig $configOnTransmit
}

proc build.buttons w {
	set f [smallfont]
	global transmitButton
	set transmitButton $w.send
	checkbutton $w.send -text "Transmit" \
		-relief raised -command transmit \
		-anchor w -variable transmitButtonState -font $f \
		-state disabled -highlightthickness 0
	button $w.release -text "Release" \
		-relief raised -command release_device \
		-font $f -highlightthickness 0

	pack $w.send $w.release -fill both
}

proc doNothing { args } {
}

proc set_bps { w value } {
	if [have grabber] {
		grabber bps $value
	#XXX
		session data-bandwidth $value
	}
	$w configure -text "$value kbps"
}

proc set_fps { w value } {
	grabber fps $value
	$w configure -text "$value fps"
}

proc build.sliders w {
	set f [smallfont]

	global V
	set key $V(session)
	global ftext btext
	set ftext($key) "0.0 f/s"
	set btext($key) "0.0 kb/s"

	frame $w.info
	label $w.info.label -text "Rate Control" -font $f
	label $w.info.fps -textvariable ftext($key) -width 6 \
		-font $f -pady 0 -borderwidth 0
        label $w.info.bps -textvariable btext($key) -width 8 \
		-font $f -pady 0 -borderwidth 0
	pack $w.info.label -side left
	pack $w.info.bps $w.info.fps -side right
	
	frame $w.bps
	scale $w.bps.scale -orient horizontal -font $f \
		-showvalue 0 -from 10 -to [option get . maxbw Vic] \
		-command "set_bps $w.bps.value" -width 12 \
		-relief groove
	label $w.bps.value -font $f -width 8 -anchor w

	frame $w.fps
	scale $w.fps.scale -font $f -orient horizontal \
		-showvalue 0 -from 1 -to 30 \
		-command "set_fps $w.fps.value" -width 12 \
		-relief groove
	label $w.fps.value -font $f -width 8 -anchor w

	pack $w.info -fill x
	pack $w.bps $w.fps -fill x
	pack $w.bps.scale -side left -fill x -expand 1
	pack $w.bps.value -side left -anchor w 
	pack $w.fps.scale -fill x -side left -expand 1
	pack $w.fps.value -side left -anchor w
	
	$w.bps.scale set [option get . bandwidth Vic]
	$w.fps.scale set [option get . framerate Vic]

	global fps_slider bps_slider
	set fps_slider $w.fps.scale
	set bps_slider $w.bps.scale
}

proc attribute_class { attr class } {
	while { [llength $attr] >= 2 } {
		if { [lindex $attr 0] == $class } {
			return [lindex $attr 1]
		}
		set attr [lrange $attr 2 end]
	}
	return ""
}

proc inList { item L } {
	foreach v $L {
		if { $v == $item } {
			return 1
		}
	}
	return 0
}

#
# Returns true iff device named by $device has an attribute named
# $attr in the attribute class $class.  For example, 
# "device_supports vl size cif" would return true.
# if $attr is "*", then returns true iff the indicated $class
# exists in the attribute list (for example, "device_supports $d port *"
# says whether a device supports port selection at all)
#
proc device_supports { device class attr } {
	set L [$device attributes]
	set L [attribute_class $L $class]
	if { $attr == "*" } {
		if { $L == "" } {
			return 0
		} else {
			return 1
		}
	} else {
		return [inList $attr $L]
	}
}

# device_formats: any type of stream produce-able from this device
# (not necessarily native)
proc device_formats device {
	set L [$device attributes]
	set sizes [attribute_class $L size]
	set formats [attribute_class $L format]
	set fmtList ""
	if [inList 422 $formats] {
		set fmtList "$fmtList nv nvdct cellb jpeg raw"
	}
	if [inList 411 $formats] {
		set fmtList "$fmtList bvc"
	}
	if [inList cif $sizes] {
		set fmtList "$fmtList h261 h263+"
	}
	if [inList jpeg $formats] {
		set fmtList "$fmtList jpeg"
		global useJPEGforH261
		if $useJPEGforH261 {
			set fmtList "$fmtList h261"
		}
	}
	return $fmtList
}

#
# called when selecting a new device: insert a grabber control panel
# if it exists and remove the old one (if one was installed)
#
proc insert_grabber_panel devname {
	set k [string first - $devname]
	if { $k >= 0 } {
		incr k -1
		set devname [string range $devname 0 $k]
	}
	set w .menu.$devname
	global grabberPanel
	if [info exists grabberPanel] {
		if { "$grabberPanel" == "$w" } {
			return
		}
		pack forget $grabberPanel
		unset grabberPanel
	}
	if { [info procs build.$devname] != "" } {
		if ![winfo exists $w] {
			frame $w
			build.$devname $w
		}
		pack $w -before .menu.encoder -padx 6 -fill x
		set grabberPanel $w
	}
}

#
# Called when use selects a particular device (like videopix or xil)
# (and at startup for default device)
#
proc select_device device {
	global transmitButton sizeButtons portButton formatButtons \
		videoFormat defaultFormat lastDevice defaultPort inputPort \
		transmitButtonState

	#
	# Remember settings of various controls for previous device
	# and close down the device if it's already open
	#
	set wasTransmitting $transmitButtonState
	if [info exists lastDevice] {
		set defaultFormat($lastDevice) $videoFormat
		set defaultPort($lastDevice) $inputPort
		release_device
	}
	set lastDevice $device

	set fmtList [device_formats $device]
	foreach b $formatButtons {
		set fmt [lindex [$b configure -value] 4]
		#XXX
		if { $fmt == "bvc" && ![yesno enableBVC] } {
			$b configure -state disabled
		} elseif { [inList $fmt $fmtList] } {
			$b configure -state normal
		} else {
			$b configure -state disabled
		}
	}
	$transmitButton configure -state normal
	if [device_supports $device size small] {
		$sizeButtons.b0 configure -state normal
	} else {
		$sizeButtons.b0 configure -state disabled
	}
	if [device_supports $device size large] {
		$sizeButtons.b2 configure -state normal
	} else {
		$sizeButtons.b2 configure -state disabled
	}
	if [device_supports $device port *] {
		$portButton configure -state normal
		attach_ports $device
	} else {
		$portButton configure -state disabled
	}

	insert_grabber_panel [$device nickname]

	set videoFormat $defaultFormat($device)
	select_format $videoFormat
	if $wasTransmitting {
		$transmitButton invoke
	}
}

proc build.device w {
	set f [smallfont]

	set m $w.menu
	menubutton $w -menu $m -text Device... \
		-relief raised -width 10 -font $f
	menu $m

	global defaultFormat inputDeviceList videoFormat
	set videoFormat [option get . defaultFormat Vic]
	if { $videoFormat == "h.261" } {
		set videoFormat h261
	}
	#
	# Disabled the device button if we have no devices or
	# if we don't have transmit persmission.
	#
	if { ![info exists inputDeviceList] || ![have_transmit_permission] } {
		$w configure -state disabled
		return
	}
	foreach d $inputDeviceList {
		if { [$d nickname] == "still" && ![yesno stillGrabber] } {
			set defaultFormat($d) $videoFormat
			continue
		}
		# this is fragile
		$m add radiobutton -label [$d nickname] \
			-command "select_device $d" \
			-value $d -variable videoDevice -font $f
		if { "[$d attributes]" == "disabled" } {
			$m entryconfigure [$d nickname] -state disabled
		}
		set fmtList [device_formats $d]
		if [inList $videoFormat $fmtList] {
			set defaultFormat($d) $videoFormat
		} else {
			set defaultFormat($d) [lindex $fmtList 0]
		}
	}
}

proc format_col { w n0 n1 n2 } {
	set f [smallfont]
	frame $w
	radiobutton $w.b0 -text $n0 -relief flat -font $f -anchor w \
		-variable videoFormat -value $n0 -padx 0 -pady 0 \
		-command "select_format $n0" -state disabled
	radiobutton $w.b1 -text $n1 -relief flat -font $f -anchor w \
		-variable videoFormat -value $n1 -padx 0 -pady 0 \
		-command "select_format $n1" -state disabled
	radiobutton $w.b2 -text $n2 -relief flat -font $f -anchor w \
		-variable videoFormat -value $n2 -padx 0 -pady 0 \
		-command "select_format $n2" -state disabled
	pack $w.b0 $w.b1 $w.b2 -fill x 

	global formatButtons
	lappend formatButtons $w.b0 $w.b1 $w.b2
}

proc build.format w {
	format_col $w.p0 nv nvdct cellb 
	format_col $w.p1 jpeg h261 bvc
	format_col $w.p2 h263+ h263 raw

	set f [smallfont]

	frame $w.glue0
	frame $w.glue1

	pack $w.glue0 -side left -fill x -expand 1
	pack $w.p0 $w.p1 $w.p2 -side left
	pack $w.glue1 -side left -fill x -expand 1

}

proc build.size w {
	set f [smallfont]

	set b $w.b
	frame $b
	radiobutton $b.b0 -text "small" -command "grabber decimate 4" \
		-padx 0 -pady 0 \
		-anchor w -variable inputSize -font $f -relief flat -value 4
	radiobutton $b.b1 -text "normal" -command "grabber decimate 2" \
		-padx 0 -pady 0 \
		-anchor w -variable inputSize -font $f -relief flat -value 2
	radiobutton $b.b2 -text "large" -command "grabber decimate 1" \
		-padx 0 -pady 0 \
		-anchor w -variable inputSize -font $f -relief flat -value 1
	pack $b.b0 $b.b1 $b.b2 -fill x 
	pack $b -anchor c -side left
	global inputSize sizeButtons
	set inputSize 2
	set sizeButtons $b
}

proc build.port w {
	set f [smallfont]
	# create the menubutton but don't defer the menu creation until later
	menubutton $w -menu $w.menu -text Port... \
		-relief raised -width 10 -font $f -state disabled
	global portButton inputPort
	set portButton $w
	set inputPort undefined
}

proc attach_ports device {
	global portButton inputPort defaultPort
	catch "destroy $portButton.menu"
	set portnames [attribute_class [$device attributes] port]
	set f [smallfont]
	set m $portButton.menu
	menu $m
	foreach port $portnames {
		$m add radiobutton -label $port \
			-command "grabber port $port" \
			-value $port -variable inputPort -font $f
	}
	if ![info exists defaultPort($device)] {
		set nn [$device nickname]
		if [info exists defaultPort($nn)] {
			set defaultPort($device) $defaultPort($nn)
		} else {
			set s [resource defaultPort($nn)]
			if { $s != "" } {
				set defaultPort($device) $s
			} else {
				set defaultPort($device) [lindex $portnames 0]
			}
		}
	}
	set inputPort $defaultPort($device)
}

proc build.type w {
	set f [smallfont]

	set m $w.menu
	menubutton $w -text Signal... -menu $m -relief raised \
		-width 10 -font $f -state disabled
	menu $m
	$m add radiobutton -label "auto" -command restart \
		-value auto -variable inputType -font $f
	$m add radiobutton -label "NTSC" -command restart \
		-value ntsc -variable inputType -font $f
	$m add radiobutton -label "PAL" -command restart \
		-value pal -variable inputType -font $f
	$m add radiobutton -label "SECAM" -command restart \
		-value secam -variable inputType -font $f

	global inputType typeButton
	set inputType auto
	set typeButton $w
}

proc build.encoder_buttons w {
	set f [smallfont]
	build.encoder_options $w.options
	build.device $w.device
	build.port $w.port
	pack $w.device $w.port $w.options -fill x
}

proc build.encoder_options w {
	global useJPEGforH261 tcl_platform useHardwareComp
	set useJPEGforH261 [yesno useJPEGforH261]
	set useHardwareComp [yesno useHardwareComp]
	set f [smallfont]
	set m $w.menu
	menubutton $w -text Options... -menu $m -relief raised -width 10 \
		-font $f
	menu $m
    	$m add checkbutton -label "Sending Slides" \
		-variable sendingSlides -font $f -command setFillRate
    	$m add checkbutton -label "Use JPEG for H261" \
		-variable useJPEGforH261 -font $f -command restart
		$m add checkbutton -label "Use Hardware Encode" \
		-variable useHardwareComp -font $f -command restart
		if {$tcl_platform(platform) == "windows"} {
			$m add checkbutton -label "Configure on Transmit" \
			-variable configOnTransmit -font $f
		}
}

proc build.tile w {
	set f [smallfont]
	set m $w.menu
	menubutton $w -text Tile... -menu $m -relief raised -width 10 \
		-font $f
	menu $m
	$m add radiobutton -label Single -command "redecorate 1" \
		-value 1 -variable V(ncol) -font $f
	$m add radiobutton -label Double -command "redecorate 2" \
		-value 2 -variable V(ncol) -font $f
	$m add radiobutton -label Triple -command "redecorate 3" \
		-value 3 -variable V(ncol) -font $f
	$m add radiobutton -label Quad -command "redecorate 4" \
		-value 4 -variable V(ncol) -font $f
}

proc build.decoder_options w {
	set f [smallfont]
	set m $w.menu
	menubutton $w -text Options... -menu $m -relief raised -width 10 \
		-font $f
	menu $m
    	$m add checkbutton -label "Mute New Sources" \
		-variable V(muteNewSources) -font $f
    	$m add checkbutton -label "Use Hardware Decode" \
		-variable V(useHardwareDecode) -font $f
	$m add separator
    	$m add command -label "Optimize Colormap" \
		-command fork_histtolut -font $f

	global V
	set V(optionsMenu) $m
	if ![have dither] {
		$m entryconfigure "Optimize Colormap" -state disabled
	}
}

proc build.external w {
	set f [smallfont]
	set m $w.menu
	global outputDeviceList
	if ![info exists outputDeviceList] {
		set outputDeviceList ""
	}
	if { [llength $outputDeviceList] <= 1 } {
		button $w -text External -relief raised \
			-width 10 -font $f -highlightthickness 0 \
			-command "extout_select $outputDeviceList"
	} else {
		menubutton $w -text External... -menu $m -relief raised \
			-width 10 -font $f 
		menu $m
		foreach d $outputDeviceList {
			$m add command -font $f -label [$d nickname] \
				-command "extout_select $d"
		}
	}
	if { $outputDeviceList == "" } {
		$w configure -state disabled
	}
}

proc build.dither w {
	set f [smallfont]
	if [have dither] {
		set var V(dither)
		set state normal
	} else {
		set var dummyDither
		set state disabled
	}
	set v $w.h0
	frame $v
	radiobutton $v.b0 -text "Ordered" -command set_dither \
		-padx 0 -pady 0 \
		-anchor w -variable $var -state $state \
		-font $f -relief flat -value od
	radiobutton $v.b1 -text "Error Diff" -command set_dither \
		-padx 0 -pady 0 \
		-anchor w -variable $var -state $state \
		-font $f -relief flat -value ed
	set v $w.h1
	frame $v
	radiobutton $v.b2 -text Quantize -command set_dither \
		-padx 0 -pady 0 \
		-anchor w -variable $var -state $state \
		-font $f -relief flat \
		-value quantize
	radiobutton $v.b3 -text Gray -command set_dither \
		-padx 0 -pady 0 \
		-anchor w -variable $var -state $state \
		-font $f -relief flat -value gray

	pack $w.h0.b0 $w.h0.b1 -anchor w -fill x
	pack $w.h1.b2 $w.h1.b3 -anchor w -fill x
	pack $w.h0 $w.h1 -side left
}

proc update_gamma { w s } {
	global V win_src
	set cm $V(colorModel)
	if ![$cm gamma $s] {
		return -1
	}
	set V(gamma) $s
	$cm free-colors
	if ![$cm alloc-colors] {
		revert_to_gray
	}
	#
	# Need to update all the windows.  Can't just do a redraw
	# on all the windows because that won't cause the renderer's
	# to update their copy of the image (which has the old colormap
	# installed).  Instead, go through all the active decoders and
	# force them to update all the windows.
	# 
	foreach src [session active] {
		set d [$src handler]
		if { $d != "" } {
			$d redraw
		}
	}

	return 0
}

proc build.gamma w {
	global V
	frame $w
	label $w.label -text "Gamma: " -font [smallfont] -anchor e
	mk.entry $w update_gamma $V(gamma)
	$w.entry configure -width 6
	if ![have dither] {
		$w.entry configure -state disabled -foreground gray60
		$w.label configure -foreground gray60
	}
	pack $w.label -side left
	pack $w.entry -side left -expand 1 -fill x -pady 2
}

proc build.decoder w {
	set f [smallfont]

	label $w.title -text Display
	frame $w.f -relief sunken -borderwidth 2

	set v $w.f.h0
	frame $v

	build.external $v.ext
	build.tile $v.tile
	build.decoder_options $v.options

	pack $v.options $v.tile $v.ext -fill x -expand 1

	set v $w.f.h2
	frame $v
	frame $v.dither -relief groove -borderwidth 2
	build.dither $v.dither
	frame $v.bot
	build.gamma $v.bot.gamma
	label $v.bot.mode -text "\[[winfo depth .top]-bit\]" -font $f
	pack $v.bot.gamma $v.bot.mode -side left -padx 4
	pack $v.dither $v.bot -anchor c -pady 2

	pack $w.f.h0 -side left -padx 6 -pady 6
	pack $w.f.h2 -side left -padx 6 -pady 6 -fill x -expand 1

	pack $w.title $w.f -fill x
}

proc build.encoder w {
	label $w.title -text Encoder
	frame $w.f -relief sunken -borderwidth 2

	frame $w.f.h0 -relief flat
	frame $w.f.h1 -relief flat
	frame $w.f.h0.eb -relief flat
	frame $w.f.h0.format -relief groove -borderwidth 2
	frame $w.f.h0.size -relief groove -borderwidth 2
	frame $w.f.h0.gap -relief flat -width 4

	build.encoder_buttons $w.f.h0.eb
	build.format $w.f.h0.format
	build.size $w.f.h0.size

	build.q $w.f.h1

	pack $w.f.h0.eb -side left -anchor n -fill y -padx 6 -pady 4
	pack $w.f.h0.format -side left -anchor n -fill both -expand 1
	pack $w.f.h0.size -side left -anchor c -fill both
	pack $w.f.h0.gap -side left -anchor c

	pack $w.f.h0 -fill x -pady 4
	pack $w.f.h1 -fill x -pady 6
	pack $w.title $w.f -fill x
}

proc jpeg_setq value {
	global useHardwareComp
	incr value
	if { $value > 95 } {
		set value 95
	} elseif { $value < 5 } {
		set value 5
	}
	
	if { $useHardwareComp } {
		grabber q $value
	} elseif [have grabber] {
			encoder q $value
	}	
	
	global qvalue
	$qvalue configure -text $value
}

proc h261_setq value {
	set value [expr int((1 - $value / 100.) * 29) + 1]
	if [have grabber] {
		encoder q $value
	}
	global qvalue
	$qvalue configure -text $value
}

proc h263+_setq value {
	set value [expr int((1 - $value / 100.) * 29) + 1]
	if [have grabber] {
		encoder q $value
	}
	global qvalue
	$qvalue configure -text $value
}

proc nv_setq value {
	set value [expr (100 - $value) / 10]
	if [have grabber] {
		encoder q $value
	}
	global qvalue
	$qvalue configure -text $value
}

proc nvdct_setq value {
	nv_setq $value
}

proc raw_setq value {
	set value [expr int((1 - $value / 100.) * 29) + 1]
	if [have grabber] {
		encoder q $value
	}
	global qvalue
	$qvalue configure -text $value
}

set bvc_quantizer(0) { 0 0 0 0 1 1 1 1 2 2 }
set bvc_quantizer(1) { 0 0 0 1 1 1 1 1 2 3 }
set bvc_quantizer(2) { 0 0 0 1 1 1 1 2 3 3 }
set bvc_quantizer(3) { 0 0 0 1 1 1 2 2 4 4 }
set bvc_quantizer(4) { 0 0 0 2 2 2 3 3 4 4 }
set bvc_quantizer(5) { 0 0 0 2 2 2 3 4 4 4 }
set bvc_quantizer(6) { 0 0 0 2 2 2 4 4 5 5 }
set bvc_quantizer(7) { 0 0 0 2 3 3 4 5 5 5 }
set bvc_quantizer(8) { 0 0 0 2 3 4 6 6 6 6 }
set bvc_quantizer(9) { 2 2 2 3 6 5 7 7 7 7 }


proc bvc_setq value {
	set value [expr 9 - $value / 10]
	if [have grabber] {
		global bvc_quantizer
		set n 0
		foreach q $bvc_quantizer($value) {
			encoder q $n [expr 7 - $q]
			incr n
		}
	}
	global qvalue
	$qvalue configure -text $value
}

#
# If the capture device is open, close it.  If transmission
# was active fire it up again.  Some state can only be set at
# device open time, so some controls must resort to this proc.
#
proc restart { } {
	if [have grabber] {
		global transmitButtonState videoDevice V
		if $transmitButtonState {
			$V(grabber) send 0
			close_device
			transmit
		} else {
			close_device
		}
	}
}

proc disable_large_button { } {
	global sizeButtons inputSize
	if { $inputSize == 1 } {
		set inputSize 2
	}
	$sizeButtons.b2 configure -state disabled
}

proc enable_large_button { } {
	global sizeButtons videoDevice
	if { [info exists videoDevice] && \
		[device_supports $videoDevice size large] } {
		$sizeButtons.b2 configure -state normal
	}
}

set qscale_val(h261) 68
set qscale_val(h263) 68
set qscale_val(nv) 80
set qscale_val(nvdct) 80
set qscale_val(bvc) 60
set qscale_val(jpeg) 29
set qscale_val(raw) 1
set lastFmt ""

proc select_format fmt {
	global qscale qlabel videoFormat qscale_val lastFmt

	if { $fmt == "h261" } {
		# H.261 supports only QCIF/CIF
		disable_large_button
	} else {
		enable_large_button
	}

	set qscale_val($lastFmt) [$qscale get]
	set lastFmt $videoFormat

	set proc $fmt\_setq
	if [inList $proc [info commands *_setq]] {
		$qscale configure -state normal -command $proc
		$qlabel configure -foreground black
	} else {
		$qscale configure -state disabled 
		$qlabel configure -foreground gray40
	}
	if [info exists qscale_val($fmt)] {
		$qscale set $qscale_val($fmt)
	}
	if [have grabber] {
		global V
		set encoder [create_encoder $videoFormat]
		set ff [$encoder frame-format]
		if { "$ff" == "[$V(encoder) frame-format]" } {
			#
			# new framer has the same format as the
			# old one.  just replace the old one without
			# re-initializing the grabber.
			# XXX doens't work if title-maker is installed
			#
			delete $V(encoder)
			set V(encoder) $encoder
			$encoder transmitter $V(session)
			$V(grabber) target $encoder
		} else {
			#
			# Restart the grabber.
			#
			delete $encoder
			restart
		}
	}
}

proc init_grabber { grabber } {
	global V configOnTransmit tcl_platform

	if {$tcl_platform(platform) == "windows"} {
		$grabber useconfig $configOnTransmit
	}

	if { [$grabber need-capwin] && ![have capwin] } {
		#
		# we need to create a window for input devices that
		# require capturing to the frame buffer.  create but
		# don't map it until after we call "$grabber decimate"
		# to specify it's size
		#
		toplevel .capture -class Vic
		wm title .capture "Video Capture Window"
		$grabber create-capwin .capture.video
		set V(capwin) .capture.video
		pack .capture.video
	    
		# capture window shouldn't be covered
		bind .capture <Visibility> "raise .capture"
	}

	$grabber transmitter $V(session)
	global qscale inputSize fps_slider bps_slider videoDevice
	$grabber fps [$fps_slider get]
	$grabber bps [$bps_slider get]
	$grabber decimate $inputSize
	if { [lindex [$qscale configure -state] 4] == "normal" } {
		set cmd [lindex [$qscale configure -command] 4]
		$cmd [$qscale get]
	}
	global inputPort inputType portButton typeButton
	if { [$portButton cget -state] == "normal" } {
		$grabber port $inputPort
	}
	setFillRate
	update
}

proc build.q w {
	set f [smallfont]
	frame $w.tb
	label $w.title -text "Quality" -font $f -anchor w
	label $w.tb.value -text 0 -font $f -width 3
	scale $w.tb.scale -font $f -orient horizontal \
		-showvalue 0 -from 0 -to 99 \
		-width 12 -relief groove
	global qscale qvalue qlabel
	set qscale $w.tb.scale
	set qvalue $w.tb.value
	set qlabel $w.title

	pack $w.tb.scale -side left -fill x -expand 1
	pack $w.tb.value -side left
	pack $w.title -padx 2 -side left
	pack $w.tb -fill x -padx 6 -side left -expand 1
}

proc build.xmit w {
	set f [smallfont]
	label $w.label -text Transmission
	frame $w.frame -relief sunken -borderwidth 2
	pack $w.label -fill x
	pack $w.frame -fill both -expand 1

	frame $w.frame.buttons
	build.buttons $w.frame.buttons
	frame $w.frame.right
	build.sliders $w.frame.right

	pack $w.frame.buttons -side left -padx 6 
	pack $w.frame.right -side right -expand 1 -fill x -padx 10 -anchor c
}

proc set_dither {} {
	global win_src
	set wlist [array names win_src]
	foreach w $wlist {
		set ws($w) $win_src($w)
		detach_window $win_src($w) $w
	}
	if ![init_color] {
		revert_to_gray
	}
	foreach w $wlist {
		attach_window $ws($w) $w
	}
}	

proc revert_to_gray {} {
	global V
	if { $V(dither) == "gray" } {
		#XXX
		puts stderr "vic: out of colors"
		exit 1
	}
	open_dialog "ran out of colors; reverting to gray"
	set V(dither) gray
	set_dither
}

