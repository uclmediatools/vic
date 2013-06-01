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

set helpno 0
proc helpitem { w text } {
	global helpno
	set f [option get . helpfont Vic]
	set h $w.h$helpno
	incr helpno
	frame $h
	canvas $h.bullet -width 12 -height 12 
	$h.bullet create oval 6 3 12 9 -fill black
	message $h.msg -justify left -anchor w -font $f -width 460 -text $text
	pack $h.bullet -side left -anchor ne -pady 5
	pack $h.msg -side left -expand 1 -fill x -anchor nw
	pack $h -expand 1 -fill both
}

proc build.help { } {
	set w .help
	if {[string match [ windowingsystem] "aqua"]} {
		::tk::unsupported::MacWindowStyle style $w plainDBox help {closeBox resizable}
	}
	if [winfo exists $w] { return }
	create_toplevel $w "vic help"

	frame $w.frame -borderwidth 2 -relief raised
	set p $w.frame
	helpitem $p "Transmit video by clicking on the \"Transmit\" button \
in the \"Menu\" window.  You need video capture hardware to do this."
	helpitem $p "Incoming video streams appear in the main vic window.  \
If you see the message \"Waiting for video...\", then no one is transmitting \
video to the conference address you're running on.  Otherwise, you'll \
see a thumbnail sized image and accompanying information for each source. \
Click on the thumbnail to open a larger viewing window.  You can tile the \
thumbnails in multiple columns using the \"Tile\" menu in the \"Menu\" window."
	helpitem $p "Clicking on the \"mute\" button for a given source will \
turn off decoding.  It is usually a good idea to do \
this for your own, looped-back transmission."
	helpitem $p "The transmission rate is controlled with the bit-rate \
and frame-rate sliders in the \"Transmission\" panel of the \"Menu\" window.  \
The more restrictive setting limits the transmission rate."
	helpitem $p "The video windows need not be fixed to a given source. \
The \"Mode...\" menu attached to a viewing window allows you to specify \
voice-switched and/or timer-switched modes.   In timer-switched mode, the \
window automatically cycles through (unmuted) sources, while in \
voice-switched mode, the window switches to whomever is talking \
(using cues from vat).  You can have more than one voice-switched window, \
which results in a simple LRU allocation of the windows to most recent \
speakers.  See the man page for more details."
	helpitem $p "If the user interface looks peculiar, you might \
have X resources that conflict with tk.  A common problem is \
defining \"*background\" and/or \"*foreground\"."
	helpitem $p "Bugs and suggestions to vic@cs.ucl.ac.uk.  Thanks."

	button $w.frame.ok -text " Dismiss " -borderwidth 2 -relief raised \
		-command "wm withdraw $w" -font [mediumfont] 
	pack $w.frame.ok -pady 6 -padx 6 -anchor e

	pack $w.frame -expand 1 -fill both
}

