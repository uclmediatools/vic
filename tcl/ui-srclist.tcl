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

#XXX allow all/or non-senders to be shown
proc build.srclist { } {
	set w .srclist
	toplevel $w
	wm withdraw $w
	wm geometry $w 300x320
	#bind $w <Enter> "focus $w"
	#XXX
	wm title $w "VIC Members"
	wm minsize $w 0 0
	global srclist
	frame $w.b -borderwidth 2 -relief sunken
	scrollbar $w.b.scroll -relief groove -borderwidth 2 \
			-command "$w.b.list yview"
	#
	# Note that we make the canvas dimensions small so that
	# if the user resizes the window, the dismiss button
	# doesn't disappear.  The default size is set by the
	# wm geometry command above.
	#
	canvas $w.b.list -relief groove -borderwidth 0 \
		-height 10 -width 10 -yscrollcommand "$w.b.scroll set" 

	set srclist $w.b.list

	button $w.ok -text " Dismiss " -borderwidth 0 -relief raised \
		-command "wm withdraw $w" -font [mediumfont] 

	pack $w.b -fill both -expand 1
	pack $w.b.scroll -side left -fill y
	pack $w.b.list -side left -expand 1 -fill both
	pack $w.ok -fill x
}
	
set srclist_bottom 2
#
# Register a src in the src display.  This happens for all the
# srcs participating in the conferencing (e.g., including
# those not actually sending video).
#
# This also sets up the layers within each source object
#
proc register src {
	global srcstate srclist srclist_bottom nametag numLayers

	set srcstate($src) 1

	set layer 0
	$src layer $layer [new SourceLayer]

	# Create SourceLayer objs within the src as required
	while { $numLayers > $layer } {
		incr layer
		$src layer $layer [new SourceLayer]
	}
	
	if [info exists srclist] {
		set f [mediumfont]
		set nametag($src) [ $srclist create text \
			5 $srclist_bottom -font $f -text [$src addr] \
				-anchor nw ]
		set srclist_bottom \
			[ lindex [ $srclist bbox $nametag($src) ] 3 ]
		incr srclist_bottom 2
		$srclist config -scrollregion "0 0 2.5i $srclist_bottom"
	}
	invoke_source_callback register $src
}

proc adjustNames { thresh h } {
	global nametag srclist srclist_bottom
	foreach s [array names nametag] {
		set y [lindex [$srclist coords $nametag($s)] 1]
		if { $y > $thresh } {
			$srclist move $nametag($s) 0 -$h
		}
	}
	incr srclist_bottom -$h
	$srclist config -scrollregion "0 0 2.5i $srclist_bottom"
}

#
# Remove a src from the src display.  This happens when
# a party quits or has been dead for a sufficiently long time.
# We assume the src has already been deactivated.
#
proc unregister src {
	global name_line info_line nametag srclist
	invoke_source_callback unregister $src

	destroy_rtp_stats $src

	#
	# Delete name_line and info_line if they exist.
	# They might not be created until the first NAME
	# report (and they aren't used until a source is activated).
	#
	if [info exists name_line($src)] {
		unset name_line($src)
		unset info_line($src)
	}
	set thresh [lindex [$srclist coords $nametag($src)] 1]
	set bb [$srclist bbox $nametag($src)]
	set height [expr [lindex $bb 3] - [lindex $bb 1]]
	incr height 2
	if [info exists srclist] {
		$srclist delete $nametag($src)
		unset nametag($src)
		adjustNames $thresh $height
	}
}

proc grayout src {
	global nametag srclist srcstate
	if [info exists srclist] {
		$srclist itemconfigure $nametag($src) -stipple gray50
	}
	set srcstate($src) 2
}

proc embolden src {
	global nametag srclist
	if [info exists srclist] {
		$srclist itemconfigure $nametag($src) -stipple {}
	}
	set srcstate($src) 1
}

