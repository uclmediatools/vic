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

set icons(file_open) [image create photo -format gif  -data { R0lGODlhEgASAPIAAAAAAICAAMDAwPj8APj8+AAAAAAAAAAAACH5BAEAAAIALAAAAAASABIAAAM7KLrc/jAKQCUDC2N7t6JeA2YDMYDoA5hsWYZk28LfjN4b4AJB7/ue1elHDOl4RKAImQzQcDeOdEp1JAAAO///}]

proc build.titlemaker w {

	#global transparency
	global selectedFile xcoord ycoord id icons	
	set selectedFile [pwd]	
	append selectedFile "/"
	append selectedFile "ucl_logo.ppm"
	
	set resourceFile [resource overlayFilename]
	if { $resourceFile != ""} {
		set selectedFile $resourceFile
	}

	set xcoord [resource overlayX]
	set ycoord [resource overlayY]
	#set transparency 255
	set id logo1

	set f [smallfont]

	#bind $w <q> "logo_quit"
	#bind $w <Enter> "focus %W"
	#bind $w <Enter> "focus $w"
	
	frame $w.b 
	
	###################### Filename ##############################
	frame $w.b.file 
	label $w.b.file.label -text "Overlay Image file:" -font $f -anchor w
	mk.entry $w.b.file tm.update.filename $selectedFile
	$w.b.file.entry configure -width 39
	if {$::tk_version > 8.4 && [windowingsystem] != "x11"} {
		ttk::button $w.b.file.selectfile \
			-compound image \
			-command "logo_fileselect $w.b.file.entry" \
			-image $icons(file_open)
	} else {
		button $w.b.file.selectfile \
			-relief raised -command "logo_fileselect $w.b.file.entry" \
			-font $f -highlightthickness 0 \
			-image $icons(file_open)
	}
	pack $w.b.file.label  -side left -anchor w -fill x -expand 1
	pack $w.b.file.entry $w.b.file.selectfile -side left -padx 2 -anchor w -fill x -expand 1
	pack $w.b.file -side top

	###################### X/Y coords #############################
    frame $w.b.xy 

	frame $w.b.xy.x
	label $w.b.xy.x.label -text "X:" -font $f -anchor w -width 2
	mk.entry $w.b.xy.x tm.update.coordx $xcoord
	$w.b.xy.x.entry configure -width 5
	pack $w.b.xy.x.label $w.b.xy.x.entry -side left -padx 3

	frame $w.b.xy.y
	label $w.b.xy.y.label -text "Y:" -font $f -anchor w -width 2
	mk.entry $w.b.xy.y tm.update.coordy $ycoord
	$w.b.xy.y.entry configure -width 5
	pack $w.b.xy.y.label $w.b.xy.y.entry -side left -padx 3

	pack $w.b.xy.x $w.b.xy.y -side left -anchor w
	pack $w.b.xy -side top

	###################### Transparency #############################
	#frame $w.b.tupper -relief sunken
	#label $w.b.tupper.value -font $f -width 8 

	#frame $w.b.tlower -relief sunken
	#scale $w.b.tlower.scale -font $f -orient horizontal \
	#	-showvalue 0 -from 0 -to 255 \
	#	-command "tm.update.transparency $w.b.tinfo.label" -width 8 \
	#	-sliderlength 15 \
	#	-relief groove
	#$w.b.tlower.scale set $transparency
	#pack $w.b.tupper.value $w.b.tlower.scale -fill x -side left -expand 1
	
	#global transparency_slider
	#set transparency_slider $w.b.transparency.scale

	#frame $w.b.transparency -relief sunken
	#frame $w.b.tinfo -relief sunken
	#label $w.b.tinfo.label -text "Transparency" -font $f
	#pack $w.b.tupper $w.b.tlower 
	#pack $w.b.tinfo.label -side left -expand 1
	##################################################################
	#pack $w.b.transparency $w.b.tinfo -side left -fill x -expand 1
	
	pack $w.b -fill both -expand 1
}


proc logo_fileselect { w } {
	global selectedFile
	
	set types {
	    {{PPM Files} {.ppm} }
	    {{All Files} *      }
	}
	set selectedFile [tk_getOpenFile -filetypes $types]
	if {$selectedFile != ""} {	    
	    puts "Got file: $selectedFile"
	}

	tm.update.filename $w $selectedFile
}

proc logo_transmit {} {
	global logoButtonState selectedFile xcoord ycoord id
	#global transparency

	if ![info exists tmEnable] {
		tm_enable
	}

	if { $logoButtonState && [file exists $selectedFile] } {
		ppm_load $id $selectedFile .\\ppmlogo1
		#if ![tm_check $id ] {
		#}
		
		#tm_transparent $id $transparency
		#after 1000 "float_graphic $id"
		
		position_graphic $id $xcoord $ycoord 0
		
	} else {
		tm_disable
	}

}


#proc tm.update.transparency { w value } {
#	global id transparency
	
#	$w configure -text "Transparency: $value"
#	set transparency $value
#	tm_transparent $id $value
#}


proc tm.update.coordx {w xval} {
	global xcoord ycoord logoButtonState id
	
	if { $xval != ""} {
	    set xcoord $xval
	    if $logoButtonState {
			position_graphic $id $xcoord $ycoord 0
		}
		return 0
	}
	return -1
}

proc tm.update.coordy {w yval} {
	global xcoord ycoord logoButtonState id
	
	if { $yval != ""} {
	    set ycoord $yval
	    if $logoButtonState {
			position_graphic $id $xcoord $ycoord 0
		}
		return 0
	}
	return -1
}


proc tm.update.filename { w fname } {
	global selectedFile

	if { $fname != ""} {
		$w delete 0 end
		$w insert 0 $fname
		set selectedFile $fname
		return 0
	}
	return -1
}



proc ppm_load { object infile outfile } {

	set ppm [new ppm]
	
	$ppm load $infile
	$ppm dump-yuv $outfile
	
	global object_width object_height object_yuv 
	set object_width($object) [$ppm width]
	set object_height($object) [$ppm height]
	set object_yuv($object) $outfile
	delete $ppm
	
	tm_create $object $outfile $object_width($object) $object_height($object)
}



proc place_graphic { o x y depth } {
	global object_x object_y object_depth
	
	set object_x($o) $x
	set object_y($o) $y
	set object_depth($o) $depth
	
	tm_place $o $x $y $depth
}
	

proc position_graphic { o x y depth } {
	#global object_width object_height
	
	#set w $object_width($o)
	#set h $object_height($o)
	#set nx [expr $x - $w / 2]
	
	#if { $nx < 0 } {
	#	set nx 0
	#}
	
	#set ny [expr $y - $h / 2]
	#if { $ny < 0 } {
	#	set ny 0
	#}
	
	#place_graphic $o $nx $ny $depth
	place_graphic $o $x $y $depth
}


proc float_graphic o {
	global object_x object_y object_depth
	
	if [info exists object_x($o)] {
		set y $object_y($o)
		if { $y > 4 } {
			incr y -4
		}
		place_graphic $o $object_x($o) $y $object_depth($o)
	}
	after 300 "float_graphic $o"
}


proc logo_quit {} {
	global id

	tm_disable
}


###############################################################

